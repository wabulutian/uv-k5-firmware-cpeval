#include "satpass.h"

#define FREQ_CORR(X)	(X+(satpass_params.data.bk4819TuneFreqOffset * 10))

static bool isInitialized;
static bool preventKeyDown;
static bool isTxEnable;
static bool isTransmitting;
static bool isListening;
static bool backlightState = true;
static bool isCWSending = false;
static uint32_t initialFreq, RxFreq, TxFreq;
static enum_State currentState, previousState;
static BK4819_FilterBandwidth_t listenBw = BK4819_FILTER_BW_WIDE;
static ModulationType modulationType = MOD_FM;
static enum_QuickMenuState quickMenuState;

#ifdef RSSITEST
#pragma region <<RSSI REC>>
#define CW_SAMPLE_LENGTH	512
static uint8_t rssiRecord[CW_SAMPLE_LENGTH] = {0};
static uint16_t rssiReadPtr = 0;
static uint16_t rssiWritePtr = 0;
static uint16_t rssiPdf[64] = {0};
static uint8_t rssiTh = 0;
static uint8_t validSignalRecord[CW_SAMPLE_LENGTH] = {0};
static uint16_t bitLengthPdf[32] = {0};	// 2 sample point per PDF cell
static uint8_t bitLengthTh = 9;
static uint8_t cw_T = 6;
static uint8_t cw_3T = 18;
static float cw_wpm = 0;
static bool isCwProcessing = false;
static bool cwProcessReq = false;
static uint8_t cwDecodeList[10] = {0};
static uint8_t cwDecodeMaskList[10] = {0};
static char cwDecodeASCIIList[21] = {0};
static uint8_t cwDecodeASCIIPos[10] = {0};
static uint8_t rssiThOffset = 20;
static bool isCwQuickMenuEnable = false;
static uint8_t txRestoreTime = 2;
#pragma endregion
#endif

const uint8_t freqChangeStep[] = {100, 50, 10}; //FM AM USB
const char* transponderTypeOptionDisp[] = {"NORMAL", "INVERSE"};
const char* freqTrackModeOptionDisp[] = {"FULLAUTO", "SEMIAUTO"};

static uint8_t tickCnt = 0;

st_SatpassSettings satpass_settings = 
{
	.currentChannel = CH_RX,
	.transponderType = TRANSPONDER_NORMAL,
	.freqTrackMode = FREQTRACK_FULLAUTO,
	.txMode = TX_FM,
	.ctcssToneIdx = 0
};

un_SatpassParams satpass_params = 
{
	.data.chkbit = 0xBEEF,
	.data.bk4819TuneFreqOffset = 0,
	.data.satpassStroageNum = 0
};

static KEY_Code_t btn;
static uint8_t btnPrev;
static uint8_t btnDownCounter;

st_Time satpass_rtc_dateTime;

static uint32_t satpass_tempFreq;
char satpass_dateTimeInputString[15] = "-------------\0"; // YYYYMMDDhhmmss\0
char satpass_freqInputString[12] = "---.--- --0\0";
uint8_t satpass_dateTimeInputIndex = 0;
KEY_Code_t satpass_dateTimeInputArr[14];
uint8_t satpass_freqInputIndex = 0;
KEY_Code_t satpass_freqInputArr[8];

static int abs(int value) {
	return value < 0 ? -value : value;
}
#ifdef RSSITEST
#pragma region <<RSSI Record>>

static void RSSI_Append(uint8_t dbm)
{
	rssiRecord[rssiWritePtr] = dbm;
	rssiWritePtr ++;
	rssiReadPtr = rssiWritePtr;
	rssiWritePtr = rssiWritePtr % CW_SAMPLE_LENGTH;
	rssiReadPtr = rssiReadPtr % CW_SAMPLE_LENGTH;
}

static uint8_t RSSI_Read(uint16_t idx)
{
	idx += rssiReadPtr;
	idx = idx % CW_SAMPLE_LENGTH;
	return rssiRecord[idx];
}

static void RSSI_FindRSSITh()
{
	memset(rssiPdf, 0, 64);
	for (int i = 0; i < CW_SAMPLE_LENGTH; i ++)
	{
		rssiPdf[rssiRecord[i] / 4] += 1;
	}
	rssiTh = 255;
	for (int i = 0; i < 63; i ++)
	{
		if (rssiPdf[i] > rssiPdf[i+1])
		{
			rssiTh = i * 4;
			break;
		}
	}
}

static void CW_StroageValidSignal()
{
	isCwProcessing = true;
	memset(validSignalRecord, 0, CW_SAMPLE_LENGTH);
	for (int i = 1; i < CW_SAMPLE_LENGTH - 1; i ++)
	{
		uint8_t tmp = RSSI_Read(i);
		validSignalRecord[i] = tmp > (rssiTh + rssiThOffset) ? 1 : 0;
	}
	isCwProcessing = false;
}

static void CW_FindLengthTh()
{
	uint16_t startPtr = 0;
	uint16_t endPtr = 0;
	uint16_t len = 0;
	uint8_t maxLen = 0;
	while (true)
	{
		if (validSignalRecord[endPtr] == 0)	// Find next bit
		{
			endPtr ++;
			startPtr = endPtr;
			if (endPtr == CW_SAMPLE_LENGTH-1) return;	// End
		}
		else
		{
			endPtr ++;
			len = endPtr - startPtr;
			len = Clamp(len, 1, 63);
			if (len > maxLen) maxLen = len;
			if (endPtr == CW_SAMPLE_LENGTH-1) break;	// End
		}
	}

	if (maxLen > 9 && maxLen < 38)	// 10~40wpm
	{
		cw_3T = maxLen;
		cw_T = cw_3T / 3;
		bitLengthTh  = cw_3T * 10 / 15;
	}
}

static void CW_Decode()
{
	uint16_t startPtr = 0;
	uint16_t endPtr = 0;
	uint16_t len = 0;
	uint8_t code = 0;
	uint8_t codeMask = 0;
	uint8_t codeNum = 0;
	memset(cwDecodeList, 0, 10);
	memset(cwDecodeMaskList, 0, 10);
	memset(cwDecodeASCIIPos, 0, 10);
	while (len < bitLengthTh * 2 || validSignalRecord[endPtr] == 0)
	{
		// If a bit is valid from the beginning, the bit is incomplete and discarded.
		if (validSignalRecord[endPtr] == 1)
		{
			endPtr ++;
			startPtr = endPtr;
			if (endPtr == CW_SAMPLE_LENGTH-1) return;	// No valid bit
		}
		else// If a bit begins within a bitLengthTh, the record starts at the middle of a character, the character is discarded
		{
			endPtr ++;
			if (endPtr == CW_SAMPLE_LENGTH-1) return;	// No valid bit
		}
		len = endPtr - startPtr;
	}
	// Here the first incomplete character is skipped and the first valid bit is found
	startPtr = endPtr;
	while (true)
	{
		// Here a valid bit is found
		while (validSignalRecord[endPtr] == 1)	// Find the end of current bit
		{
			endPtr ++;
			if (endPtr == CW_SAMPLE_LENGTH-1) return;	// If a bit is valid until the end, the character is incomplete and discarded.
		}
		// Here the end of a bit is found
		len = endPtr - startPtr - 1;
		startPtr = endPtr;
		codeMask |= 1;
		// Is di or da
		if (len >= bitLengthTh) code |= 1;
		// Here a bit is stroaged
		while (validSignalRecord[endPtr] == 0)	// Find next bit
		{
			endPtr ++;
			if (endPtr == CW_SAMPLE_LENGTH-1)
			{
				len = endPtr - startPtr;
				if (len >= bitLengthTh)
				{
					cwDecodeList[codeNum] = code;
					cwDecodeMaskList[codeNum] = codeMask;
					cwDecodeASCIIPos[codeNum] = startPtr / 4;
				}
				return;
			}
		}
		len = endPtr - startPtr;
		// Check in-character space or inter-character space
		if (len > bitLengthTh)
		{
			cwDecodeList[codeNum] = code;
			cwDecodeMaskList[codeNum] = codeMask;
			cwDecodeASCIIPos[codeNum] = startPtr / 4;
			code = 0;
			codeMask = 0;
			codeNum ++;
			if (codeNum == 10) return;
		}
		else
		{
			code <<= 1;
			codeMask <<= 1;
		}
		startPtr = endPtr;
	}
}

static void CW_FindASCII()
{
	for (int i = 0; i < 10; i ++)
	{
		cwDecodeASCIIList[i*2] = ' ';
		for (int j = 0; j < 46; j ++)
		{
			if (cwDecodeList[i] == cwLUT[j].code &&
				cwDecodeMaskList[i] == cwLUT[j].mask)
			{
				cwDecodeASCIIList[i*2] = cwLUT[j].ascii;
			}
		}
	}
}

static void CW_ToggleQuickMenu(bool increase)
{
	if (modulationType == MOD_FM)
	{
		uint8_t tmp = satpass_settings.ctcssToneIdx;
		if (tmp == 0 && !increase) tmp = 50;
		else tmp = increase ? (tmp + 1) : (tmp - 1);
		tmp = tmp % 51;
		satpass_settings.ctcssToneIdx = tmp;
	}
	else
	{
		int8_t tmp = satpass_params.data.bk4819TuneFreqOffset;
		tmp = increase ? (tmp + 1) : (tmp - 1);
		tmp = Clamp(tmp, -10, 10);		// in measurements.h by @Fagci
		satpass_params.data.bk4819TuneFreqOffset = tmp;
	}
}

#pragma endregion
#endif

#pragma region <<UART>>
int8_t satpass_uartRprt = 0;
un_PktData satpass_uart_pktData;
un_PktRprt satpass_uart_pktRprt;
static void Satpass_UART_RxHandler()
{
    // Check RXTO bit to confirm whether UART is idle
    if (((UART1->IF & UART_IF_RXTO_MASK) >> UART_IF_RXTO_SHIFT) == 0x1)
	{
        // Clear RXTO bit
		UART1->IF |= UART_IF_RXTO_BITS_SET;
        // Process rx data
		satpass_uartRprt = Satpass_UART_IsValidMsg();
		if (satpass_uartRprt == SATPASS_MSG_TYPE_RPRT)
		{
            // Handler for RPRT
		}
		else if (satpass_uartRprt == SATPASS_MSG_TYPE_DATA)
		{
            // Handler for DATA
		}
        else if(satpass_uartRprt != -1)
		{
			PktSend_RPRT(satpass_uartRprt);
		}
	}
}

#pragma endregion

#pragma region <<RTC>>

int IsLeapYear(int year){
  	return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static void Satpass_SetCurrentTime(uint16_t yr, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec){
	const uint8_t daysInMonth[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
	long long totalSeconds = 0;
	for (int y = 1970; y < yr; y ++){
		totalSeconds += IsLeapYear(y) ? 31622400 : 31536000;
	}
	for (int m =  1; m < mon; m ++){
		totalSeconds += daysInMonth[m] * 86400;
		if (m == 2 && IsLeapYear(yr)){
		totalSeconds += 86400;
		}
	}
	totalSeconds += (day - 1) * 86400 + hour * 3600 + min * 60 + sec;

	satpass_rtc_dateTime.year = yr;
	satpass_rtc_dateTime.month = mon;
	satpass_rtc_dateTime.day = day;
	satpass_rtc_dateTime.hour = hour;
	satpass_rtc_dateTime.min = min;
	satpass_rtc_dateTime.sec = sec;
	satpass_rtc_dateTime.tv = totalSeconds;
	
}

static void Satpass_RTC_ReadDateTime(){
	uint16_t year = satpass_rtc_dateTime.year;
	uint8_t month = satpass_rtc_dateTime.month;;
	uint8_t day = satpass_rtc_dateTime.day;
	uint8_t hour = satpass_rtc_dateTime.hour;
	uint8_t min = satpass_rtc_dateTime.min;
	uint8_t sec = satpass_rtc_dateTime.sec;

	RTC_ReadDateTime(&year, &month, &day, &hour, &min, &sec);

	Satpass_SetCurrentTime(year + 2000, month, day, hour, min, sec);
}

static void Satpass_RTC_SetDateTime(uint16_t yr, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec){
	yr -= 2000;
	RTC_SetDateTime(yr/10, yr%10, mon/10, mon%10, day/10, day%10, hour/10, hour%10, min/10, min%10, sec/10, sec%10);
	RTC_LoadDateTime();
}
#pragma endregion

#pragma region <<TIM>>

static void Satpass_TIM_Init()
{
	// Enable TIMB0 and interrupt
	// See DP32G030 user manual for reg def
	NVIC_EnableIRQ(DP32_TIMER_BASE0_IRQn);
	(*(uint32_t*)(0x40064000U + 0x04U)) = 47U;
	(*(uint32_t*)(0x40064000U + 0x20U)) = 9999U;	// 10ms timer for normal app but idk it's accuary or not. Whatever it looks fine
	(*(uint32_t*)(0x40064000U + 0x10U)) |= 0x2;		// Enable interrupt
	(*(uint32_t*)(0x40064000U + 0x00U)) |= 0x2;		// Enable TIMB0
}

static void Satpass_TIM_DeInit()
{
	NVIC_DisableIRQ(DP32_TIMER_BASE0_IRQn);
	(*(uint32_t*)(0x40064000U + 0x00U)) &= 0x0;
	(*(uint32_t*)(0x40064000U + 0x14U)) |= 0x2;
	(*(uint32_t*)(0x40064000U + 0x10U)) &= 0x0;
}

#pragma endregion

#pragma region <<BK4819>>
/**
 * BK4819 Operations From @Fagci
*/
static bool audioState = true;
static uint8_t scanRssi;
static uint16_t registersBackup[128];
static uint16_t registersValue[128] = {0};
static const uint8_t registersToBackup[] = {
    0x13, 0x30, 0x31, 0x37, 0x3D, 0x40, 0x43, 0x47, 0x48, 0x7D, 0x7E,
};

static void ToggleAudio(bool on) {
	if (on == audioState) {
		return;
	}
	audioState = on;
	if (on) {
		GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
	} else {
		GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
	}
}

static void ToggleAFDAC(bool on) {
	uint32_t Reg = BK4819_ReadRegister(BK4819_REG_30);
	Reg &= ~(1 << 9);
	if (on)
		Reg |= (1 << 9);
	BK4819_WriteRegister(BK4819_REG_30, Reg);
}

uint16_t GetBWRegValue() { 
	return listenBWRegValues[listenBw]; 
}

static uint8_t GetRssi() {
	return Clamp(BK4819_GetRSSI(), 0, 255);
}

static void Measure() {
	scanRssi = GetRssi(); 
}

static void ToggleAFBit(bool on) {
	uint16_t reg = BK4819_ReadRegister(BK4819_REG_47);
	reg &= ~(1 << 8);
	if (on)
		reg |= on << 8;
	BK4819_WriteRegister(BK4819_REG_47, reg);
}

static void RegBackupSet(uint8_t num, uint16_t value) {
  registersValue[num] = BK4819_ReadRegister(num);
  BK4819_WriteRegister(num, value);
}

static void RegRestore(uint8_t num) {
  BK4819_WriteRegister(num, registersValue[num]);
}

static void ToggleTX(bool);
static void ToggleRX(bool);

static void ToggleRX(bool on) {
	if (isListening == on) {
		return;
	}
	
	isListening = on;
	if (on) {
		ToggleTX(false);
	}

	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, on);
	BK4819_RX_TurnOn();

	ToggleAudio(on);
	BK4819_ToggleAFDAC(on);
	BK4819_ToggleAFBit(on);
	BK4819_WriteRegister(0x43, GetBWRegValue());
}

/**
 * 	Enable the Tx channel but mute MIC, for CW mode
 * 	(No voice input = Carrier only = CW)
*/
static void TxArmed(bool on)
{
	if (isTransmitting == on) {
		return;
	}
	isTransmitting = on;
	
	if (on) {
		ToggleRX(false);
	}

	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_RED, on);

	if (on) {
		BK4819_TuneTo(FREQ_CORR(TxFreq));

		RegBackupSet(BK4819_REG_47, 0x6040);
		RegBackupSet(BK4819_REG_7E, 0x302E);
		RegBackupSet(BK4819_REG_50, 0xBB20);
		RegBackupSet(BK4819_REG_37, 0x1D0F);
		RegBackupSet(BK4819_REG_52, 0x028F);
		RegBackupSet(BK4819_REG_30, 0x0000);
		RegBackupSet(BK4819_REG_51, 0x0000);

		BK4819_SetupPowerAmplifier(gCurrentVfo->TXP_CalculatedSetting,
								FREQ_CORR(TxFreq));
	}
	else 
	{

		BK4819_SetupPowerAmplifier(0, 0);

		RegRestore(BK4819_REG_51);
		BK4819_WriteRegister(BK4819_REG_30, 0);
		RegRestore(BK4819_REG_30);
		RegRestore(BK4819_REG_52);
		RegRestore(BK4819_REG_37);
		RegRestore(BK4819_REG_50);
		RegRestore(BK4819_REG_7E);
		RegRestore(BK4819_REG_47);

		BK4819_TuneTo(FREQ_CORR(RxFreq));
	}
	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2, !on);
  	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1, on);
}

/**
 *  Original Tx swtch from @Fagci. Enable all Tx chain
*/
static void ToggleTX(bool on) {
	if (isTransmitting == on) {
		return;
	}
	isTransmitting = on;
	
	if (on) {
		ToggleRX(false);
	}

	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_RED, on);

	if (on) {
		ToggleAudio(false);

		BK4819_TuneTo(FREQ_CORR(TxFreq));

		RegBackupSet(BK4819_REG_47, 0x6040);
		RegBackupSet(BK4819_REG_7E, 0x302E);
		RegBackupSet(BK4819_REG_50, 0x3B20);
		RegBackupSet(BK4819_REG_37, 0x1D0F);
		RegBackupSet(BK4819_REG_52, 0x028F);
		RegBackupSet(BK4819_REG_30, 0x0000);
		BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
		RegBackupSet(BK4819_REG_51, 0x0000);
		if (satpass_settings.ctcssToneIdx != 0)
		{
			BK4819_SetCTCSSFrequency(CTCSS_Options[satpass_settings.ctcssToneIdx - 1]);
		}

		BK4819_SetupPowerAmplifier(gCurrentVfo->TXP_CalculatedSetting,
								FREQ_CORR(TxFreq));
	} else {

		RADIO_SendEndOfTransmission();
		RADIO_EnableCxCSS();

		BK4819_SetupPowerAmplifier(0, 0);

		RegRestore(BK4819_REG_51);
		BK4819_WriteRegister(BK4819_REG_30, 0);
		RegRestore(BK4819_REG_30);
		RegRestore(BK4819_REG_52);
		RegRestore(BK4819_REG_37);
		RegRestore(BK4819_REG_50);
		RegRestore(BK4819_REG_7E);
		RegRestore(BK4819_REG_47);

		BK4819_TuneTo(FREQ_CORR(RxFreq));
	}
	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2, !on);
  	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1, on);
}

static void BackupRegisters() {
  for (uint8_t i = 0; i < ARRAY_SIZE(registersToBackup); ++i) {
    uint8_t regNum = registersToBackup[i];
    registersBackup[regNum] = BK4819_ReadRegister(regNum);
  }
}

static void RestoreRegisters() {
  for (uint8_t i = 0; i < ARRAY_SIZE(registersToBackup); ++i) {
    uint8_t regNum = registersToBackup[i];
    BK4819_WriteRegister(regNum, registersBackup[regNum]);
  }
}

static void ToggleModulation() {
  if (modulationType == MOD_RAW) {
    modulationType = MOD_FM;
  } else {
    modulationType++;
  }
  BK4819_SetModulation(modulationType);
  if (modulationType != MOD_USB)	satpass_settings.txMode = TX_FM; // Only DSB can use CW
}

static void ToggleIFBW() {
	if (listenBw == BK4819_FILTER_BW_NARROWER) {
		listenBw = BK4819_FILTER_BW_WIDE;
	} else {
		listenBw++;
	}
	BK4819_WriteRegister(0x43, GetBWRegValue());
}

#pragma endregion

#pragma region <<Utils>>

static bool IsFreqLegal(uint32_t f)
{
	if (f > 14400000 && f < 14800000) return true;
	if (f > 42000000 && f < 45000000) return true;
	return false;
}

void SetState(enum_State state) {
  previousState = currentState;
  currentState = state;
}

static void UpdateBatteryInfo() 
{
  for (uint8_t i = 0; i < 4; i++) 
  {
    BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[i], &gBatteryCurrent);	
  }

  uint16_t voltage = Mid(gBatteryVoltages, ARRAY_SIZE(gBatteryVoltages));	// in measurements.h by @Fagci
  gBatteryDisplayLevel = 0;

  for (int i = ARRAY_SIZE(gBatteryCalibration) - 1; i >= 0; --i) 
  {
    if (gBatteryCalibration[i] < voltage) 
	{
      gBatteryDisplayLevel = i + 1;
      break;
    }
  }
}

static void Satpass_ResetFreqInput()
{
	satpass_tempFreq = 0;
	for (int i = 0; i < 10; ++i) 
	{	// xxx.--- --0
		satpass_freqInputString[i] = '-';
	}
	satpass_freqInputString[3] = '.';
	satpass_freqInputString[7] = ' ';
	satpass_freqInputString[10] = '0';
}

static void Satpass_FreqInput() {
	satpass_freqInputIndex = 0;
	Satpass_ResetFreqInput();
	SetState(FREQ_INPUT);
}

static void Satpass_SetFreq(uint32_t f)
{
	if (satpass_settings.currentChannel == CH_RX || satpass_settings.currentChannel == CH_RXMASTER)
	{
		BK4819_TuneTo(FREQ_CORR(f));
		RxFreq = f;
	}
	else if (satpass_settings.currentChannel == CH_TX || satpass_settings.currentChannel == CH_TXMASTER)
	{
		TxFreq = f;
	}

	if (satpass_settings.currentChannel == CH_RXMASTER) 
	{
		TxFreq = f;
	}
	else if (satpass_settings.currentChannel == CH_TXMASTER) 
	{
		BK4819_TuneTo(FREQ_CORR(f));
		RxFreq = f;
	}
}

static void UpdateCurrentFreqNormal(bool inc) {
	uint8_t offset = freqChangeStep[modulationType];
	uint32_t f;
	if (satpass_settings.currentChannel == CH_RX || satpass_settings.currentChannel == CH_RXMASTER) f = RxFreq;
	else f = TxFreq;
	if (inc && f < F_MAX) {
		f += offset;
	} else if (!inc && f > F_MIN) {
		f -= offset;
	}
	if (satpass_settings.currentChannel == CH_RX || satpass_settings.currentChannel == CH_RXMASTER)
	{
		BK4819_TuneTo(FREQ_CORR(f));
		RxFreq = f;
	}
	else if (satpass_settings.currentChannel == CH_TX || satpass_settings.currentChannel == CH_TXMASTER)
	{
		TxFreq = f;
	}

	if (satpass_settings.currentChannel == CH_RXMASTER) 
	{
		TxFreq = f;
	}
	else if (satpass_settings.currentChannel == CH_TXMASTER) 
	{
		BK4819_TuneTo(FREQ_CORR(f));
		RxFreq = f;
	}
}

// Another channel
static void UpdateCurrentFreqAux(bool inc) {
	if (satpass_settings.currentChannel == CH_RXMASTER || satpass_settings.currentChannel == CH_TXMASTER) return;
	uint8_t offset = freqChangeStep[modulationType];
	uint32_t f;
	if (satpass_settings.currentChannel == CH_TX) f = RxFreq;
	else f = TxFreq;
	if (inc && f < F_MAX) {
		f += offset;
	} else if (!inc && f > F_MIN) {
		f -= offset;
	}
	if (satpass_settings.currentChannel == CH_TX) {
		BK4819_TuneTo(FREQ_CORR(f));
		RxFreq = f;
	}
	if (satpass_settings.currentChannel == CH_RX) {
		TxFreq = f;
	}
}

static void Satpass_UpdateFreqInput(KEY_Code_t key) {
	if (key != KEY_EXIT && satpass_freqInputIndex >= 8) {
		return;
	}
	if (key == KEY_EXIT) {
		satpass_freqInputIndex--;
	} else if (key <= KEY_9) {
		satpass_freqInputArr[satpass_freqInputIndex++] = key;
	}

	Satpass_ResetFreqInput();

	KEY_Code_t digitKey;
	for (int i = 0; i < 3; ++i) {	// xxx.--- --0
		if (i < satpass_freqInputIndex) {
			digitKey = satpass_freqInputArr[i];
			satpass_freqInputString[i] = '0' + digitKey;
		} else {
			satpass_freqInputString[i] = '-';
		}
	}
	for (int i = 4; i < 7; ++i) {	// ---.xxx --0
		if (i < satpass_freqInputIndex + 1) {
			digitKey = satpass_freqInputArr[i-1];
			satpass_freqInputString[i] = '0' + digitKey;
		} else {
			satpass_freqInputString[i] = '-';
		}
	}
	for (int i = 8; i < 10; ++i) {	// ---.xxx --0
		if (i < satpass_freqInputIndex + 2) {
			digitKey = satpass_freqInputArr[i-2];
			satpass_freqInputString[i] = '0' + digitKey;
		} else {
			satpass_freqInputString[i] = '-';
		}
	}
	satpass_tempFreq = 0;
	uint32_t base = 10000000; // 100MHz in BK units
	for (int i = 0; i < satpass_freqInputIndex; ++i) {
		satpass_tempFreq += satpass_freqInputArr[i] * base;
		base /= 10;
	}
}

static void Satpass_ToggleChannel() 
{
	#ifdef RSSITEST
		satpass_settings.currentChannel = satpass_settings.currentChannel == CH_RX ? CH_TX : CH_RX;
	#else
	if (satpass_settings.currentChannel != CH_TXMASTER) {
		satpass_settings.currentChannel += 1;
	} else {
		satpass_settings.currentChannel = CH_RX;
	}
	#endif
}

static void Satpass_ToggleTxMode()
{
	if (modulationType == MOD_USB) satpass_settings.txMode = satpass_settings.txMode == TX_FM ? TX_CW : TX_FM;
}

static void Satpass_ToggleQuickMenuValue(enum_QuickMenuState state, bool increase)
{
	switch (state) {
	case MENU_TRANSPONDER:
		satpass_settings.transponderType = 
			satpass_settings.transponderType == TRANSPONDER_NORMAL ? TRANSPONDER_INVERSE : TRANSPONDER_NORMAL;
		break;
	case MENU_FREQTRACK:
		satpass_settings.freqTrackMode = 
			satpass_settings.freqTrackMode == FREQTRACK_FULLAUTO ? FREQTRACK_SEMIAUTO : FREQTRACK_FULLAUTO;
		break;
	case MENU_3RD:
		if (modulationType == MOD_FM)
		{
			uint8_t tmp = satpass_settings.ctcssToneIdx;
			if (tmp == 0 && !increase) tmp = 50;
			else tmp = increase ? (tmp + 1) : (tmp - 1);
			tmp = tmp % 51;
			satpass_settings.ctcssToneIdx = tmp;
		}
		else
		{
			int8_t tmp = satpass_params.data.bk4819TuneFreqOffset;
			tmp = increase ? (tmp + 1) : (tmp - 1);
			tmp = Clamp(tmp, -10, 10);		// in measurements.h by @Fagci
			satpass_params.data.bk4819TuneFreqOffset = tmp;
		}
		break;
	default:
		break;
	}
}

#pragma endregion

#pragma region <<UI>>
char String[32];
static void Satpass_UI_DrawMainInterface()
{
	Satpass_UI_DrawFrequency(RxFreq, TxFreq, satpass_settings.currentChannel, currentState == FREQ_INPUT, satpass_freqInputString, isTransmitting);
	Satpass_UI_DrawChannelIcon(satpass_settings.currentChannel);

	if (satpass_settings.txMode == TX_CW)
	{
		if (isCWSending)
		{
			for (int j = 0; j < 12; j ++) 
			{
				gFrameBuffer[0][105 + j] |= 0b11111000;
				gFrameBuffer[1][105 + j] |= 0b00011111;
			}
		}
		sprintf(String, "CW");
		UI_PrintStringSmallest(String, 107, 5, false, !isCWSending);
	}

	int dbm = Rssi2DBm(scanRssi);	// in measurements.h by @Fagci
	int baseDbm = abs(Rssi2DBm(rssiTh));	// in measurements.h by @Fagci
	int offsetDbm = rssiThOffset >> 1;
	uint8_t s = DBm2S(dbm);			// in measurements.h by @Fagci
	Satpass_UI_DrawRssi(dbm, s, !isTransmitting);

	#ifdef RSSITEST
	for (int i = 0; i < CW_SAMPLE_LENGTH; i ++)
	{
		uint8_t tmp0 = RSSI_Read(i);
		uint8_t tmp1 = RSSI_Read(i+1);
		tmp0 = Clamp((tmp0 - rssiTh) / 2, 0, 15);
		tmp1 = Clamp((tmp1 - rssiTh) / 2, 0, 15);
		Satpass_UI_DrawLine(i / 4, 43 - tmp0, (i+1) / 4, 43 - tmp1, true);
	}
	for (int i = 0; i < CW_SAMPLE_LENGTH; i ++)
	{
		PutPixel(i / 4, 45, validSignalRecord[i]);
	}
	sprintf(String, "-%03d+%02d", baseDbm, offsetDbm);
	UI_PrintStringSmallest(String, 98, 43 - rssiThOffset/2 - 6, false, true);
	Satpass_UI_DrawLine(0, 43 - rssiThOffset/2, 127, 43 - rssiThOffset/2, true);

	sprintf(String, "WPM:%d", 100 / cw_T * 12 / 10);
	UI_PrintStringSmallest(String, 0, 2, true,true);
	for (int i = 0; i < 10; i ++)
	{
		if (cwDecodeASCIIList[i*2] != ' ' && cwDecodeASCIIPos[i] != 0)
		{
			UI_PrintStringSmallBold(&cwDecodeASCIIList[i*2], cwDecodeASCIIPos[i], cwDecodeASCIIPos[i], 6);
		}
	}

	if (isCwQuickMenuEnable)
	{
		for (int i = 0; i < QUICKMENU_CELL_WIDTH; i ++) gStatusLine[30 + i] = 0xFF;
	}
	if (modulationType == MOD_FM)
	{
		if (satpass_settings.ctcssToneIdx != 0)
		{
			sprintf(String, "%d.%d", CTCSS_Options[satpass_settings.ctcssToneIdx - 1] / 10,
				CTCSS_Options[satpass_settings.ctcssToneIdx - 1] % 10);
		}
		else
		{
			sprintf(String, "CTCSS OFF");
		}
	}
	else
	{
		sprintf(String, satpass_params.data.bk4819TuneFreqOffset < 0 ? "-%d00Hz" : "%d00Hz", abs(satpass_params.data.bk4819TuneFreqOffset));
	}
	UI_PrintStringSmallest(String, 33, 2, true, !isCwQuickMenuEnable);
	
	#else
	if (modulationType == MOD_FM)
	{
		if (satpass_settings.ctcssToneIdx != 0)
		{
			sprintf(String, "%d.%d", CTCSS_Options[satpass_settings.ctcssToneIdx - 1] / 10,
				CTCSS_Options[satpass_settings.ctcssToneIdx - 1] % 10);
		}
		else
		{
			sprintf(String, "CTCSS OFF");
		}
	}
	else
	{
		sprintf(String, satpass_params.data.bk4819TuneFreqOffset < 0 ? "-%d00Hz" : "%d00Hz", abs(satpass_params.data.bk4819TuneFreqOffset));
	}
	Satpass_UI_DrawQuickMenu(transponderTypeOptionDisp[satpass_settings.transponderType],
							freqTrackModeOptionDisp[satpass_settings.freqTrackMode],
							String,
							quickMenuState,
							quickMenuState != MENU_NONE);

	Satpass_UI_DrawEmptyRadar();

	Satpass_UI_DrawDateTime(satpass_rtc_dateTime);
	#endif

	UpdateBatteryInfo();

	Satpass_UI_DrawStatusBar(modulationType, listenBw, gBatteryDisplayLevel);
	ST7565_BlitFullScreen();
	ST7565_BlitStatusLine();
}
#pragma endregion

#pragma region <<USER INPUT>>

/**
 *  Key value reading from @Fagci
*/
static KEY_Code_t GetKey() {
	KEY_Code_t btn = KEYBOARD_Poll();
	if (btn == KEY_INVALID && !GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT)) {
		btn = KEY_PTT;
	}
	return btn;
}

static void APP_ExitSatPass();

static void OnKeysReleased() {
	preventKeyDown = false;
	if (isTransmitting) {
		if (satpass_settings.txMode == TX_FM) ToggleTX(false);
		else TxArmed(false);
		ToggleRX(true);
		isCWSending = false;
	}
}

static void OnKeyDownNormal(KEY_Code_t key)
{
	switch (key) 
	{
	case KEY_1:
		UpdateCurrentFreqAux(true);
		break;
	case KEY_2:
		Satpass_ToggleChannel();
		break;
	case KEY_3:
		if (rssiThOffset < 40) rssiThOffset ++;
		break;
	case KEY_5:
		if (!isCwQuickMenuEnable) Satpass_FreqInput();
		break;
	case KEY_6:
		ToggleIFBW();
		break;
	case KEY_7:
		UpdateCurrentFreqAux(false);
		break;
	case KEY_8:
		Satpass_ToggleTxMode();
		break;
	case KEY_9:
		if (rssiThOffset > 2) rssiThOffset --;
		break;
	case KEY_0:
		ToggleModulation();
		break;
	case KEY_UP:
		
		#ifdef RSSITEST
		if (isCwQuickMenuEnable)
		{
			CW_ToggleQuickMenu(true);
		#else
		if (quickMenuState != MENU_NONE) 
		{
			Satpass_ToggleQuickMenuValue(quickMenuState, true);
			#endif
			break;
		}
		UpdateCurrentFreqNormal(true);
		break;
	case KEY_DOWN:
		#ifdef RSSITEST
		if (isCwQuickMenuEnable)
		{
			CW_ToggleQuickMenu(false);
		#else
		if (quickMenuState != MENU_NONE) 
		{
			CW_ToggleQuickMenu(false);
			#
			Satpass_ToggleQuickMenuValue(quickMenuState, false);
			#endif
			break;
		}
		UpdateCurrentFreqNormal(false);
		break;
	case KEY_STAR:
		break;
	case KEY_F:
		
		break;
	case KEY_SIDE1:
		//TODO
		//SetState(SAT_MENU);
		break;
	case KEY_SIDE2:
		//ToggleBacklight();
		break;
	case KEY_PTT:
		if (!IsFreqLegal(TxFreq)) break;
		if (satpass_settings.txMode == TX_FM && modulationType == MOD_FM) ToggleTX(true);
		else if (satpass_settings.txMode == TX_CW && modulationType == MOD_USB) TxArmed(true);
		break;
	case KEY_MENU:
		preventKeyDown = true;
		#ifdef RSSITEST
			isCwQuickMenuEnable = !isCwQuickMenuEnable;
		#else
		if (quickMenuState == MENU_3RD) 
		{
			quickMenuState = MENU_TRANSPONDER;
		} 
		else 
		{
			quickMenuState++;
		}
		#endif
		
		break;
	case KEY_EXIT:
		preventKeyDown = true;
		#ifdef RSSITEST
		if (!isCwQuickMenuEnable)
		{
			
			APP_ExitSatPass();
			break;
		}
		isCwQuickMenuEnable = false;
		#else
		if (quickMenuState == MENU_NONE) 
		{
			
			break;
		}
		#endif
		quickMenuState = MENU_NONE;
		
		break;
	default:
		break;
	}
}

static void OnKeyDownFreqInput(uint8_t key) 
{
	switch (key) 
	{
	case KEY_0: Satpass_UpdateFreqInput(key); break;
	case KEY_1: Satpass_UpdateFreqInput(key); break;
	case KEY_2: Satpass_UpdateFreqInput(key); break;
	case KEY_3: Satpass_UpdateFreqInput(key); break;
	case KEY_4: Satpass_UpdateFreqInput(key); break;
	case KEY_5: Satpass_UpdateFreqInput(key); break;
	case KEY_6: Satpass_UpdateFreqInput(key); break;
	case KEY_7: Satpass_UpdateFreqInput(key); break;
	case KEY_8: Satpass_UpdateFreqInput(key); break;
	case KEY_9: Satpass_UpdateFreqInput(key); break;
	case KEY_STAR:break;
	case KEY_EXIT:
		if (satpass_freqInputIndex == 0) 
		{
			SetState(previousState);
			break;
		}
		Satpass_UpdateFreqInput(key);
		break;
	case KEY_MENU:
		if (satpass_tempFreq < F_MIN || satpass_tempFreq > F_MAX) 
		{
		  break;
		}
		SetState(previousState);
		Satpass_SetFreq(satpass_tempFreq);
		break;
	default:
		break;
	}
	preventKeyDown = true;
}

static void OnKeyDownCWTx(uint8_t key) 
{
	if (key == KEY_5)
	{
		BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
		isCWSending = true;
	}
	else
	{
		BK4819_WriteRegister(BK4819_REG_30, 0);
		isCWSending = false;
	}
}

static void Satpass_UserInputHandler()
{
	btnPrev = btn;
	btn = GetKey();

	if (btn == KEY_INVALID) 
	{
		btnDownCounter = 0;
		OnKeysReleased();
		return;
	}
	if (preventKeyDown) return;
	// CW mode, [PTT] + [5] = tx, no debounce
	if (isTransmitting && satpass_settings.txMode == TX_CW) 
	{
		OnKeyDownCWTx(btn);
		return;
	}

	if (btn == btnPrev && btnDownCounter < 200) btnDownCounter++;

	if (btnDownCounter % 10 == 4 || btnDownCounter > 50) {
		switch (currentState) {
		case NORMAL:
			OnKeyDownNormal(btn);
			break;
		case FREQ_INPUT:
			OnKeyDownFreqInput(btn);
			break;
		}
		//tickMs += 1;
	}
	return;
}


#pragma endregion

static void Tick20ms()
{
	Satpass_UserInputHandler();
}

static void Tick10ms()
{
	if (isCwProcessing) return;
	if (isTransmitting)
	{
		txRestoreTime = 2;
		return;
	}
	else if (txRestoreTime > 0)
	{
		txRestoreTime --;
		return;
	}
	Measure();
	RSSI_Append(scanRssi);
}


static void Satpass_APP_BackupProcess()
{
	BackupRegisters();
}

static void Satpass_APP_Init()
{
	RxFreq = TxFreq = initialFreq =
		gEeprom.VfoInfo[gEeprom.TX_CHANNEL].pRX->Frequency;
	currentState = previousState = NORMAL;
	ToggleRX(true), ToggleRX(false); // hack to prevent noise when squelch off
  	BK4819_SetModulation(modulationType);
	BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);

	// Enable clock
	SYSCON_DEV_CLK_GATE = 	SYSCON_DEV_CLK_GATE |
                        	SYSCON_DEV_CLK_GATE_RTC_BITS_ENABLE |
                        	SYSCON_DEV_CLK_GATE_TIMER_BASE0_BITS_ENABLE;
							
	Satpass_Uart_pktDistributeArr[SATPASS_MSG_TYPE_RPRT] = satpass_uart_pktRprt.array;
	Satpass_Uart_pktDistributeArr[SATPASS_MSG_TYPE_DATA] = satpass_uart_pktData.array;
    Satpass_UART_Init();
    RTC_Init();
	RTC_Enable();
}

static void Satpass_APP_RestoreProcess()
{
	RestoreRegisters();
}

static void Satpass_APP_Deinit()
{
    Satpass_UART_DeInit();
	Satpass_TIM_DeInit();
}

static void APP_ExitSatPass()
{
	Satpass_APP_Deinit();
	Satpass_APP_RestoreProcess();
	isInitialized = false;
}

void APP_RunSatpass()
{
    Satpass_APP_BackupProcess();
    Satpass_APP_Init();
	ToggleRX(true);
	Satpass_UI_DrawMainInterface();
	SYSTICK_DelayUs(100000);
	Satpass_TIM_Init();
	isInitialized = true;
	preventKeyDown = true;
	while (isInitialized) {
		Satpass_RTC_ReadDateTime();
		Satpass_UART_RxHandler();
		
		if (!isTransmitting && cwProcessReq)
		{
			RSSI_FindRSSITh();
			CW_StroageValidSignal();
			CW_FindLengthTh();
			CW_Decode();
			CW_FindASCII();
			cwProcessReq = false;
		}
		memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
		memset(gStatusLine, 0, sizeof(gStatusLine));
		
		Satpass_UI_DrawMainInterface();
	}
}


// TIMB0 Interrupt Callback
void HandlerTIMER_BASE0(void);
void HandlerTIMER_BASE0(void)
{
	(*(uint32_t*)(0x40064000U + 0x14U)) |= 0x2;	// clear flag
	tickCnt ++;
	tickCnt = tickCnt % 50;
	Tick10ms();
	if (tickCnt % 2 == 0)	Tick20ms();
	if (tickCnt == 0) cwProcessReq = true;
}