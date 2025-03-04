#include "coprocessor/coprocessor.h"

static bool isInitialized;
static bool preventKeyDown;
static bool isListening;
static bool isTransmitting;
static bool isFirstBoot = true;
static bool isFreqBackup = false;
static BK4819_FilterBandwidth_t listenBw = BK4819_FILTER_BW_WIDE;
static ModulationType cp_modulationType = MOD_FM;
static uint32_t initialFreq;
static un_Freq mainFreq = {0};
static enum_State cp_currentState, cp_previousState;
static enum_QuickMenuState cp_quickMenuState;
static uint8_t cp_currentFreqChangeStepIdx = 0;
const uint16_t cp_freqChangeStep[] = {10000, 1000, 100, 10};
static uint8_t cp_menuSelectIdx = 0;

static uint8_t scanRssi;
static uint8_t rssiThOffset = 20;
static uint8_t rssiTh = 0;

static uint8_t txRestoreTime = 2;
static uint8_t tickCnt = 0;

static KEY_Code_t btn;
static uint8_t btnPrev;
static uint8_t btnDownCounter;

static uint32_t cp_tempFreq;
char cp_dateTimeInputString[15] = "-------------\0"; // YYYYMMDDhhmmss\0
char cp_freqInputString[12] = "---.--- --0\0";
uint8_t cp_dateTimeInputIndex = 0;
KEY_Code_t cp_dateTimeInputArr[14];
uint8_t cp_freqInputIndex = 0;
KEY_Code_t cp_freqInputArr[8];

static bool isSatpassEnable = false;
static st_Time dateTime = {0};
static un_Site site = {0};
static un_tle tleTmp = {0};
static un_Freq freqTmp = {0};
static un_SatInfo satInfoTmp;
static un_SatInfo satList[5] = {0};
#ifdef ENABLE_PASS
static un_PassInfo passInfo = {0};
static un_PassInfo passInfo_local = {0};
static uint8_t passInfoReadCD = 0;
static uint8_t satInfoReadIdx = 0;
#endif
static int8_t selectedSat = 0;
static uint8_t radarMode = 0;
static un_Freq dopplerOriginal = {0};
static un_Freq dopplerBackup = {0};
static uint8_t dopplerTuneTick = 0;

static char configBuf[256] = {0};

st_CpSettings cp_settings = 
{
	.currentChannel = CH_RX,
	.txMode = TX_FM,
	.ctcssToneIdx = 0,
	.freqTrackMode = FREQTRACK_OFF,
    .transponderType = TRANSPONDER_NORMAL,
};

const char* cp_freqTrackModeOptionDisp[] = {"OFF", "FULL", "SEMI"};
const char* cp_transponderTypeOptionDisp[] = {"NORMAL", "INVERSE"};

#pragma region <<Utils>>

static bool IsFreqLegal(uint32_t f)
{
	if (f > 13000000 && f < 15000000) return true;
	if (f > 40000000 && f < 50000000) return true;
	return false;
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

void CP_SetState(enum_State state) {
  cp_previousState = cp_currentState;
  cp_currentState = state;
}

static void CP_ResetFreqInput()
{
	cp_tempFreq = 0;
	for (int i = 0; i < 10; ++i) 
	{	// xxx.--- --0
		cp_freqInputString[i] = '-';
	}
	cp_freqInputString[3] = '.';
	cp_freqInputString[7] = ' ';
	cp_freqInputString[10] = '0';
}

static void CP_FreqInput() {
	cp_freqInputIndex = 0;
	CP_ResetFreqInput();
	CP_SetState(FREQ_INPUT);
}

static void CP_GetDopplerFreq(st_Freq original, st_Freq* result, int16_t speed_100mPs) 
{
	if (speed_100mPs > 100 || speed_100mPs < -100) return;

    int32_t diffUp = ((int32_t)original.up / 2997924 * speed_100mPs); //cc in 100m/s
	result->up = original.up + (int32_t)diffUp;

    int32_t diffDn =  ((int32_t)original.down / 2997924 * speed_100mPs);
	diffDn *= -1;
	result->down = original.down + (int32_t)diffDn;
}

#pragma endregion

#pragma region <<UI Drawing>>

char String[32];
static void CP_UI_DrawMainInterface()
{
	CP_UI_DrawFrequency(mainFreq.data.down, mainFreq.data.up, cp_settings.currentChannel, cp_settings.freqTrackMode, cp_currentState == FREQ_INPUT, cp_freqInputString, isTransmitting);
	CP_UI_DrawChannelIcon(cp_settings.currentChannel);

	int dbm = Rssi2DBm(scanRssi);	// in measurements.h by @Fagci
	int baseDbm = abs(Rssi2DBm(rssiTh));	// in measurements.h by @Fagci
	int offsetDbm = rssiThOffset >> 1;
	uint8_t s = DBm2S(dbm);			// in measurements.h by @Fagci
	CP_UI_DrawRssi(dbm, s, !isTransmitting);

	if (cp_modulationType == MOD_FM)
	{
		if (cp_settings.ctcssToneIdx != 0)
		{
			sprintf(String, "%d.%d", CTCSS_Options[cp_settings.ctcssToneIdx - 1] / 10,
				CTCSS_Options[cp_settings.ctcssToneIdx - 1] % 10);
		}
		else
		{
			sprintf(String, "CTCSS");
		}
	}
	CP_UI_DrawQuickMenu(cp_freqTrackModeOptionDisp[cp_settings.freqTrackMode],
						cp_transponderTypeOptionDisp[cp_settings.transponderType],	
						String,
						cp_quickMenuState);

	if(isSatpassEnable)
	{
#ifdef ENABLE_PASS
		CP_UI_DrawRadar(&satList, selectedSat, passInfo_local.data, 2);
		CP_UI_DrawSatInfo(satList[selectedSat].data, passInfo_local.data);
#else
		CP_UI_DrawRadar(&satList, selectedSat, 2);
		CP_UI_DrawSatInfo(satList[selectedSat].data);
#endif
	}
	else CP_UI_DrawEmptyRadar();

	UpdateBatteryInfo();

	CP_UI_DrawStatusBar(cp_currentFreqChangeStepIdx, cp_modulationType, listenBw, gBatteryDisplayLevel, dateTime);

	ST7565_BlitFullScreen();
	ST7565_BlitStatusLine();
}

static void CP_UI_DrawMenuInterface()
{
	UpdateBatteryInfo();

	CP_UI_DrawStatusBar(99, cp_modulationType, listenBw, gBatteryDisplayLevel, dateTime);
	CP_UI_Menu_DrawSatList(cp_menuSelectIdx, satList, isSatpassEnable, selectedSat + 1);
	if(cp_menuSelectIdx != 0)
	{
#ifdef ENABLE_PASS
		CP_UI_Menu_DrawSatInfo(satList[cp_menuSelectIdx - 1].data, passInfo_local.data);
		CP_UI_DrawRadar(&satList, cp_menuSelectIdx - 1, passInfo_local.data, 2);
#else
		CP_UI_Menu_DrawSatInfo(satList[cp_menuSelectIdx - 1].data);
		CP_UI_DrawRadar(&satList, cp_menuSelectIdx - 1, 1);
#endif
	}
	else CP_UI_DrawRadar(&satList, 0, 3);

	ST7565_BlitFullScreen();
	ST7565_BlitStatusLine();
}

#pragma endregion

#pragma region <<TLE>>

static void CP_ConvertDateTime(char* input, uint16_t* year, uint8_t* month, uint8_t* day,
                 uint8_t* hour, uint8_t* min, uint8_t* sec, int8_t* zone)
{
	char* p = input;
	int sign = 1;

	// DATETIME
	*year = (*p - '0') * 1000 + (*(p+1) - '0') * 100 + (*(p+2) - '0') * 10 + (*(p+3) - '0');
	p += 4;
	*month = (*p - '0') * 10 + (*(p+1) - '0');
	p += 2;
	*day = (*p - '0') * 10 + (*(p+1) - '0');
	p += 2;
	*hour = (*p - '0') * 10 + (*(p+1) - '0');
	p += 2;
	*min = (*p - '0') * 10 + (*(p+1) - '0');
	p += 2;
	*sec = (*p - '0') * 10 + (*(p+1) - '0');
	p += 2;
	if (*p == '-') {
		sign = -1;
		p++;
	} else if (*p == '+') {
		p++;
	}
	*zone = (*p - '0') * 10 + (*(p+1) - '0');
	*zone *= sign;
	p += 2;
	while (*p != '\n')
	{
		p++;
	}
}

static void CP_ConvertStie(char* input, int32_t* lon10k, int32_t* lat10k)
{
	char* p = input;

	// LONLAT
	*lon10k = 0;
	*lat10k = 0;
	int sign = 1;
	if (*p == '-') {
		sign = -1;
		p++;
	} else if (*p == '+') {
		p++;
	}
	for (int i = 0; i < 3; i++) 
	{
		*lon10k = *lon10k * 10 + (*p++ - '0');
	}
	p++; // skip '.'
	for (int i = 0; i < 4; i++) 
	{
		*lon10k = *lon10k * 10 + (*p++ - '0');
	}
	*lon10k *= sign;

	p++; // skip space
	sign = 1;
	if (*p == '-') {
		sign = -1;
		p++;
	} else if (*p == '+') {
		p++;
	}
	for (int i = 0; i < 3; i++) 
	{
		*lat10k = *lat10k * 10 + (*p++ - '0');
	}
	p++; // skip '.'
	for (int i = 0; i < 4; i++) 
	{
		*lat10k = *lat10k * 10 + (*p++ - '0');
	}
	*lat10k *= sign;
}

static void CP_ConvertFreq(char* input, uint32_t* up, uint32_t* down)
{
	char* p = input;

	*up = 0;
	*down = 0;
	for (int i = 0; i < 6; i++) 
	{
		*up = *up * 10 + (*p++ - '0');
	}

	p++; // skip space
	for (int i = 0; i < 6; i++) 
	{
		*down = *down * 10 + (*p++ - '0');
	}
	*up *= 100;
	*down *= 100;
}

static void CP_ConvertTLE(char* input, char* l1, char* l2, char* l3) 
{
	// SAT NAME
	char* p = input;
	char* q = l1;
	while (*p >= ' ' && *p <= 'z')
	{
		*q++ = *p++;
	}
	*q = '\0';
	while (!(*p >= ' ' && *p <= 'z'))
	{
		p++;
	}

	// TLE
	q = l2;
	while (*p >= ' ' && *p <= 'z')
	{
		*q++ = *p++;
	}
	*q = '\0';
	while (!(*p >= ' ' && *p <= 'z'))
	{
		p++;
	}

	q = l3;
	while (*p >= ' ' && *p <= 'z')
	{
		*q++ = *p++;
	}
	*q = '\0';
}

#pragma endregion

#pragma region <<TIM>>

static void CP_TIM_Init()
{
	// Enable TIMB0 and interrupt
	// See DP32G030 user manual for reg def
	NVIC_EnableIRQ(DP32_TIMER_BASE0_IRQn);
	(*(uint32_t*)(0x40064000U + 0x04U)) = 47U;
	(*(uint32_t*)(0x40064000U + 0x20U)) = 9999U;	// 10ms timer for normal app but idk it's accuary or not. Whatever it looks fine
	(*(uint32_t*)(0x40064000U + 0x10U)) |= 0x2;		// Enable interrupt
	(*(uint32_t*)(0x40064000U + 0x00U)) |= 0x2;		// Enable TIMB0
}

static void CP_TIM_DeInit()
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
	BK4819_SetFilterBandwidth(listenBw);
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

		BK4819_TuneTo((mainFreq.data.up));

		RegBackupSet(BK4819_REG_47, 0x6040);
		RegBackupSet(BK4819_REG_7E, 0x302E);
		RegBackupSet(BK4819_REG_50, 0x3B20);
		RegBackupSet(BK4819_REG_37, 0x1D0F);
		RegBackupSet(BK4819_REG_52, 0x028F);
		RegBackupSet(BK4819_REG_30, 0x0000);
		BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
		
		RegBackupSet(BK4819_REG_51, 0x0000);
		if (cp_settings.ctcssToneIdx != 0)
		{
			BK4819_SetCTCSSFrequency(CTCSS_Options[cp_settings.ctcssToneIdx - 1]);
		}
		else
		{
			BK4819_ExitSubAu();
		}

		BK4819_SetupPowerAmplifier(gCurrentVfo->TXP_CalculatedSetting,
								(mainFreq.data.up));
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

		BK4819_TuneTo((mainFreq.data.down));
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
  if (cp_modulationType == MOD_USB) {
    cp_modulationType = MOD_FM;
  } else {
    cp_modulationType++;
  }
  BK4819_SetModulation(cp_modulationType);
  if (cp_modulationType != MOD_USB)	cp_settings.txMode = TX_FM; // Only DSB can use CW
}

static void ToggleIFBW() {
	if (listenBw == BK4819_FILTER_BW_NARROWER) {
		listenBw = BK4819_FILTER_BW_WIDE;
	} else {
		listenBw++;
	}
	BK4819_SetFilterBandwidth(listenBw);
}

#pragma endregion

#pragma region <<RF Utils>>

static void CP_SetFreq(uint32_t f)
{
	if (cp_settings.currentChannel == CH_RX || cp_settings.currentChannel == CH_RXMASTER)
	{
		BK4819_TuneTo(f);
		mainFreq.data.down = f;
	}
	else if (cp_settings.currentChannel == CH_TX || cp_settings.currentChannel == CH_TXMASTER)
	{
		mainFreq.data.up = f;
	}

	if (cp_settings.currentChannel == CH_RXMASTER) 
	{
		mainFreq.data.up = f;
	}
	else if (cp_settings.currentChannel == CH_TXMASTER) 
	{
		BK4819_TuneTo(f);
		mainFreq.data.down = f;
	}
}

static void UpdateCurrentFreqNormal(bool inc) {
	uint16_t offset = cp_freqChangeStep[cp_currentFreqChangeStepIdx];
	uint32_t f;
	if (cp_settings.currentChannel == CH_RX || cp_settings.currentChannel == CH_RXMASTER) f = mainFreq.data.down;
	else f = mainFreq.data.up;
	if (inc && f < F_MAX) {
		f += offset;
	} else if (!inc && f > F_MIN) {
		f -= offset;
	}
	if (cp_settings.currentChannel == CH_RX || cp_settings.currentChannel == CH_RXMASTER)
	{
		BK4819_TuneTo(f);
		mainFreq.data.down = f;
	}
	else if (cp_settings.currentChannel == CH_TX || cp_settings.currentChannel == CH_TXMASTER)
	{
		mainFreq.data.up = f;
	}

	if (cp_settings.currentChannel == CH_RXMASTER) 
	{
		mainFreq.data.up = f;
	}
	else if (cp_settings.currentChannel == CH_TXMASTER) 
	{
		BK4819_TuneTo(f);
		mainFreq.data.down = f;
	}
}

// Another channel
static void UpdateCurrentFreqAux(bool inc) {
	if (cp_settings.currentChannel == CH_RXMASTER || cp_settings.currentChannel == CH_TXMASTER) return;
	uint16_t offset = cp_freqChangeStep[cp_currentFreqChangeStepIdx];
	uint32_t f;
	if (cp_settings.currentChannel == CH_TX) f = mainFreq.data.down;
	else f = mainFreq.data.up;
	if (inc && f < F_MAX) {
		f += offset;
	} else if (!inc && f > F_MIN) {
		f -= offset;
	}
	if (cp_settings.currentChannel == CH_TX) {
		BK4819_TuneTo(f);
		mainFreq.data.down = f;
	}
	if (cp_settings.currentChannel == CH_RX) {
		mainFreq.data.up = f;
	}
}

static void CP_UpdateFreqInput(KEY_Code_t key) {
	if (key != KEY_EXIT && cp_freqInputIndex >= 8) {
		return;
	}
	if (key == KEY_EXIT) {
		cp_freqInputIndex--;
	} else if (key <= KEY_9) {
		cp_freqInputArr[cp_freqInputIndex++] = key;
	}

	CP_ResetFreqInput();

	KEY_Code_t digitKey;
	for (int i = 0; i < 3; ++i) {	// xxx.--- --0
		if (i < cp_freqInputIndex) {
			digitKey = cp_freqInputArr[i];
			cp_freqInputString[i] = '0' + digitKey;
		} else {
			cp_freqInputString[i] = '-';
		}
	}
	for (int i = 4; i < 7; ++i) {	// ---.xxx --0
		if (i < cp_freqInputIndex + 1) {
			digitKey = cp_freqInputArr[i-1];
			cp_freqInputString[i] = '0' + digitKey;
		} else {
			cp_freqInputString[i] = '-';
		}
	}
	for (int i = 8; i < 10; ++i) {	// ---.xxx --0
		if (i < cp_freqInputIndex + 2) {
			digitKey = cp_freqInputArr[i-2];
			cp_freqInputString[i] = '0' + digitKey;
		} else {
			cp_freqInputString[i] = '-';
		}
	}
	cp_tempFreq = 0;
	uint32_t base = 10000000; // 100MHz in BK units
	for (int i = 0; i < cp_freqInputIndex; ++i) {
		cp_tempFreq += cp_freqInputArr[i] * base;
		base /= 10;
	}
}

static void CP_ToggleChannel() 
{
	if (cp_settings.currentChannel != CH_TXMASTER) {
		cp_settings.currentChannel += 1;
	} else {
		cp_settings.currentChannel = CH_RX;
	}
}

static void CP_ToggleTxMode()
{
	if (cp_modulationType == MOD_USB) cp_settings.txMode = cp_settings.txMode == TX_FM ? TX_CW : TX_FM;
}

static void CP_ToggleQuickMenuValue(enum_QuickMenuState state, bool increase)
{
	switch (state) {
	case MENU_FREQTRACK:
		cp_settings.freqTrackMode = 
			cp_settings.freqTrackMode == FREQTRACK_FULLAUTO ? FREQTRACK_OFF : (cp_settings.freqTrackMode + 1);
		break;
	case MENU_TRANSPONDER:
		cp_settings.transponderType = 
			cp_settings.transponderType == TRANSPONDER_NORMAL ? TRANSPONDER_INVERSE : TRANSPONDER_NORMAL;
		break;
	case MENU_3RD:
		if (cp_modulationType == MOD_FM)
		{
			uint8_t tmp = cp_settings.ctcssToneIdx;
			if (tmp == 0 && !increase) tmp = 50;
			else tmp = increase ? (tmp + 1) : (tmp - 1);
			tmp = tmp % 51;
			cp_settings.ctcssToneIdx = tmp;
		}
		else
		{
		}
		break;
	default:
		break;
	}
}
#pragma endregion

#pragma region <<USER_INPUT>>

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

static void OnKeysReleased() {
	preventKeyDown = false;
	if (isTransmitting) {
		if (cp_settings.txMode == TX_FM) ToggleTX(false);
		ToggleRX(true);
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
		CP_ToggleChannel();
		break;
	case KEY_3:
		break;
	case KEY_4:
		cp_currentFreqChangeStepIdx ++;
		cp_currentFreqChangeStepIdx = cp_currentFreqChangeStepIdx % 4;
		break;
	case KEY_5:
		if (cp_quickMenuState == MENU_NONE && !(isSatpassEnable && cp_settings.freqTrackMode != FREQTRACK_OFF)) CP_FreqInput();
		break;
	case KEY_6:
		ToggleIFBW();
		break;
	case KEY_7:
		UpdateCurrentFreqAux(false);
		break;
	case KEY_8:
		CP_ToggleTxMode();
		break;
	case KEY_9:
		break;
	case KEY_0:
		ToggleModulation();
		break;
	case KEY_UP:
		
		if (cp_quickMenuState != MENU_NONE) 
		{
			CP_ToggleQuickMenuValue(cp_quickMenuState, true);
			break;
		}
		if (!(isSatpassEnable && cp_settings.freqTrackMode != FREQTRACK_OFF)) UpdateCurrentFreqNormal(true);
		break;
	case KEY_DOWN:
		if (cp_quickMenuState != MENU_NONE) 
		{
			CP_ToggleQuickMenuValue(cp_quickMenuState, false);
			break;
		}
		if (!(isSatpassEnable && cp_settings.freqTrackMode != FREQTRACK_OFF)) UpdateCurrentFreqNormal(false);
		break;
	case KEY_STAR:
		break;
	case KEY_F:
		
		break;
	case KEY_SIDE1:
		preventKeyDown = true;
		if (cp_quickMenuState == MENU_NONE)
		{
			cp_quickMenuState = MENU_FREQTRACK;
		}
		else if (cp_quickMenuState == MENU_3RD) 
		{
			cp_quickMenuState = MENU_FREQTRACK;
		} 
		else 
		{
			cp_quickMenuState++;
		}
		
		break;
	case KEY_SIDE2:
		//ToggleBacklight();
		break;
	case KEY_PTT:
		if (!IsFreqLegal(mainFreq.data.up)) break;
		if (cp_settings.txMode == TX_FM && cp_modulationType == MOD_FM) ToggleTX(true);
		break;
	case KEY_MENU:
		preventKeyDown = true;
		CP_SetState(MENU);
		break;
	case KEY_EXIT:
		preventKeyDown = true;
		if (cp_quickMenuState == MENU_NONE) 
		{
			CP_APP_ExitCoprocessor();
			break;
		}
		cp_quickMenuState = MENU_NONE;
		break;
	default:
		break;
	}
}

static void OnKeyDownFreqInput(uint8_t key) 
{
	switch (key) 
	{
	case KEY_0: CP_UpdateFreqInput(key); break;
	case KEY_1: CP_UpdateFreqInput(key); break;
	case KEY_2: CP_UpdateFreqInput(key); break;
	case KEY_3: CP_UpdateFreqInput(key); break;
	case KEY_4: CP_UpdateFreqInput(key); break;
	case KEY_5: CP_UpdateFreqInput(key); break;
	case KEY_6: CP_UpdateFreqInput(key); break;
	case KEY_7: CP_UpdateFreqInput(key); break;
	case KEY_8: CP_UpdateFreqInput(key); break;
	case KEY_9: CP_UpdateFreqInput(key); break;
	case KEY_STAR:break;
	case KEY_EXIT:
		if (cp_freqInputIndex == 0) 
		{
			CP_SetState(cp_previousState);
			break;
		}
		CP_UpdateFreqInput(key);
		break;
	case KEY_MENU:
		if (cp_tempFreq < F_MIN || cp_tempFreq > F_MAX) 
		{
		  break;
		}
		CP_SetState(cp_previousState);
		CP_SetFreq(cp_tempFreq);
		break;
	default:
		break;
	}
	preventKeyDown = true;
}

static void OnKeyDownMenu(uint8_t key) 
{
	switch (key) 
	{
	case KEY_0: break;
	case KEY_1: break;
	case KEY_2: break;
	case KEY_3: break;
	case KEY_4: break;
	case KEY_5: break;
	case KEY_6: break;
	case KEY_7: break;
	case KEY_8: break;
	case KEY_9: break;
	case KEY_STAR: break;
	case KEY_UP:
		if (cp_menuSelectIdx == 0) cp_menuSelectIdx = 5;
		else cp_menuSelectIdx --;
	break;
	case KEY_DOWN:
		cp_menuSelectIdx ++;
		cp_menuSelectIdx = cp_menuSelectIdx % 6;
	break;
	case KEY_EXIT:
		cp_menuSelectIdx = (selectedSat + 1) * isSatpassEnable;
		CP_SetState(cp_previousState);
		break;
	case KEY_MENU:
		if (cp_menuSelectIdx == 0) isSatpassEnable = false;
		else if (satList[cp_menuSelectIdx - 1].data.valid == 1)
		{
			selectedSat = cp_menuSelectIdx - 1;
			isSatpassEnable = true;
		}
		break;
	default:
		break;
	}
#ifdef ENABLE_PASS
	passInfoReadCD = 0;
#endif
	preventKeyDown = true;
}

static void CP_UserInputHandler()
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

	if (btn == btnPrev && btnDownCounter < 200) btnDownCounter++;

	if (btnDownCounter % 15 == 4 || btnDownCounter > 150) {
		switch (cp_currentState) {
		case NORMAL:
			OnKeyDownNormal(btn);
			break;
		case FREQ_INPUT:
			OnKeyDownFreqInput(btn);
			break;
		case MENU:
			OnKeyDownMenu(btn);
			break;
		}
	}
	return;
}

#pragma endregion

static void Tick20ms()
{
	CP_UserInputHandler();
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
}

static void Tick100ms()
{
	CP_I2C_RTC(&dateTime, CP_I2C_DIR_READ);
	int8_t stat;
	stat = SProto_TLEReader(configBuf, 300);
	if (stat == 'S')
	{
		CP_ConvertStie(configBuf, &site.data.lon, &site.data.lat);
		printf("Set site: %d, %d\n\r", site.data.lon, site.data.lat);
		CP_I2C_Site(&site);
	} else if (stat == 'T')
	{
		if (cp_currentState != MENU || cp_menuSelectIdx == 0)
		{
			printf("TLE NOT SAVE!\n\rCHOOSE AN VALID SLOT AND TRY AGAIN!\n\r");
			return;
		}
		CP_ConvertTLE(configBuf, tleTmp.lines.l1, tleTmp.lines.l2, tleTmp.lines.l3);
		printf("Add sat@%d: %s\n\r%s\n\r%s\n\r", cp_menuSelectIdx, tleTmp.lines.l1, tleTmp.lines.l2, tleTmp.lines.l3);
		CP_I2C_TLE(tleTmp.tle, cp_menuSelectIdx - 1);

		for (int n = 0; n < 20; n ++)
		{
			uint8_t idx, sum = 0;
			idx = n % 5;
			SYSTEM_DelayMs(5);
			CP_I2C_SATINFO(&satInfoTmp, idx);
			for (int i = 0; i < CP_I2C_SIZE_SAT_INFO - 1; i ++)
			{
				sum += satInfoTmp.array[i];
			}
			if (sum != satInfoTmp.data.chk) n --;
			memcpy(&satList[idx].array, &satInfoTmp.array, CP_I2C_SIZE_SAT_INFO);
			printf(".", cp_menuSelectIdx, tleTmp.tle);
		}
#ifdef ENABLE_PASS
		satInfoReadIdx = cp_menuSelectIdx - 1;
		passInfoReadCD = 0;
#endif
	} else if (stat == 'D')
	{
		CP_ConvertDateTime(configBuf, &dateTime.year, &dateTime.month, &dateTime.day,
							&dateTime.hour, &dateTime.min, &dateTime.sec, &dateTime.zone);
		printf("Set time: %02d%02d%02d%02d%02d%02d %d\n\r", dateTime.year, dateTime.month, dateTime.day,
		 					dateTime.hour, dateTime.min, dateTime.sec, dateTime.zone);
		CP_I2C_RTC(&dateTime, CP_I2C_DIR_WRITE);
	} else if (stat == 'F')
	{
		if (cp_currentState != MENU || cp_menuSelectIdx == 0)
		{
			printf("FREQ NOT SAVE!\n\rCHOOSE AN VALID SLOT AND TRY AGAIN!\n\r");
			return;
		}
		CP_ConvertFreq(configBuf, &freqTmp.data.up, &freqTmp.data.down);
		CP_I2C_Freq(&freqTmp, cp_menuSelectIdx - 1);
	} else 
	{
		return;
	}
	printf("OK!\n\r");
}

static void Tick500ms()
{
	uint8_t idx, sum = 0;

	if (cp_currentState == MENU) idx = cp_menuSelectIdx - 1;
	else if(isSatpassEnable) idx = selectedSat;
	else return;

	CP_I2C_SATINFO(&satInfoTmp, idx);
	for (int i = 0; i < CP_I2C_SIZE_SAT_INFO - 1; i ++)
	{
		sum += satInfoTmp.array[i];
	}
	if (sum != satInfoTmp.data.chk) return;
	memcpy(&satList[idx].array, &satInfoTmp.array, CP_I2C_SIZE_SAT_INFO);
}

static void Tick1s()
{
	if (dopplerTuneTick > 0)
	{
		dopplerTuneTick -= 1;
		return;
	}
	if (isSatpassEnable && cp_settings.freqTrackMode != FREQTRACK_OFF)
	{
		BK4819_TuneTo(mainFreq.data.down);
		dopplerTuneTick = 5;
	}
#ifdef ENABLE_PASS
	if (!passInfoReadCD)
	{
		if (cp_currentState == MENU && cp_menuSelectIdx != 0)
		{	
			CP_I2C_PASSINFO(&passInfo, cp_menuSelectIdx - 1);
			memcpy(&passInfo_local, &passInfo, CP_I2C_SIZE_PASS_INFO);
		}
		else if(isSatpassEnable)
		{
			CP_I2C_PASSINFO(&passInfo, selectedSat);
			for (int i = 0; i < 60; i ++)
			{
				if (passInfo.data.azElList[i].azimuth > 3600) return;
				if (passInfo.data.azElList[i].azimuth < 0) return;
				if (passInfo.data.azElList[i].elevation > 900) return;
				if (passInfo.data.azElList[i].elevation < -900) return;
				if (passInfo.data.aosMin != 255 && passInfo.data.aosMin > 180) return;
			}
			memcpy(&passInfo_local, &passInfo, CP_I2C_SIZE_PASS_INFO);
		}
		passInfoReadCD = 1;
	}
	passInfoReadCD --;
#endif
}

static void CP_APP_BackupProcess()
{
	BackupRegisters();
}

static void CP_APP_RestoreProcess()
{
	RestoreRegisters();
}

static void CP_APP_Init()
{
	// Enable clock
	SYSCON_DEV_CLK_GATE = 	SYSCON_DEV_CLK_GATE |
                        	SYSCON_DEV_CLK_GATE_RTC_BITS_ENABLE |
                        	SYSCON_DEV_CLK_GATE_TIMER_BASE0_BITS_ENABLE;
	
	// Display some nb console things
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
	memset(gStatusLine, 0, sizeof(gStatusLine));
	sprintf(String, "Starting APP");
	UI_PrintStringSmallest(String, 0, 0, false, true);
	ST7565_BlitFullScreen();

	// Init local freq
	if (isFirstBoot)
	{
		mainFreq.data.down = mainFreq.data.up = initialFreq =
			gEeprom.VfoInfo[gEeprom.TX_CHANNEL].pRX->Frequency;

		dopplerBackup.data.up = mainFreq.data.up;
		dopplerBackup.data.down = mainFreq.data.down;
		isFirstBoot = false;
	}

	sprintf(String, "Reading RTC time");
	UI_PrintStringSmallest(String, 0, 8, false, true);
	ST7565_BlitFullScreen();
	// RTC
	CP_I2C_RTC(&dateTime, CP_I2C_DIR_READ);


	sprintf(String, "Reading satellite list");
	UI_PrintStringSmallest(String, 0, 16, false, true);
	ST7565_BlitFullScreen();
	// Read all satellite
	sprintf(String, ".");
	for (int n = 0; n < 20; n ++)
	{
		uint8_t idx, sum = 0;
		idx = n % 10;
		SYSTEM_DelayMs(5);
		CP_I2C_SATINFO(&satInfoTmp, idx);
		for (int i = 0; i < CP_I2C_SIZE_SAT_INFO - 1; i ++)
		{
			sum += satInfoTmp.array[i];
		}
		if (sum != satInfoTmp.data.chk) n --;
		memcpy(&satList[idx].array, &satInfoTmp.array, CP_I2C_SIZE_SAT_INFO);
		UI_PrintStringSmallest(String, n * 5, 24, false, true);
		ST7565_BlitFullScreen();
	}

	sprintf(String, "Changing MCU settings");
	UI_PrintStringSmallest(String, 0, 32, false, true);
	ST7565_BlitFullScreen();
	// Set some params
	cp_currentState = cp_previousState = NORMAL;
	ToggleRX(true), ToggleRX(false); // hack to prevent noise when squelch off
  	BK4819_SetModulation(cp_modulationType);
	BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);
}

static void CP_APP_Deinit()
{
    CP_UART_DeInit();
	CP_TIM_DeInit();
}

void CP_APP_ExitCoprocessor(void)
{
	CP_APP_Deinit();
	CP_APP_RestoreProcess();
	isInitialized = false;
}

void CP_APP_RunCoprocessor(void)
{
    CP_APP_BackupProcess();
    CP_APP_Init();
	ToggleRX(true);
	SYSTEM_DelayMs(10);
	CP_TIM_Init();
	CP_UART_Init();
	isInitialized = true;
	preventKeyDown = true;
	while (isInitialized)
	{
		memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
		memset(gStatusLine, 0, sizeof(gStatusLine));
		if (cp_currentState == MENU) CP_UI_DrawMenuInterface();
		else CP_UI_DrawMainInterface();

		if (!isSatpassEnable)
		{
			if (isFreqBackup)
			{
				mainFreq.data.up = dopplerBackup.data.up;
				mainFreq.data.down = dopplerBackup.data.down;
				BK4819_TuneTo(mainFreq.data.down);
			}
			isFreqBackup = false;
		}
		else if (!isFreqBackup)
		{
			dopplerBackup.data.up = mainFreq.data.up;
			dopplerBackup.data.down = mainFreq.data.down;
			BK4819_TuneTo(mainFreq.data.down);
			isFreqBackup = true;
		}
		if (isSatpassEnable && !isTransmitting && cp_settings.freqTrackMode != FREQTRACK_OFF)
		{
			dopplerOriginal.data.up = satList[selectedSat].data.uplinkFreq;
			dopplerOriginal.data.down = satList[selectedSat].data.downlinkFreq;
			CP_GetDopplerFreq(dopplerOriginal.data, &mainFreq.data, satList[selectedSat].data.currentSpd);
		}
	}
}

// TIMB0 Interrupt Callback
void HandlerTIMER_BASE0(void);
void HandlerTIMER_BASE0(void)
{
	(*(uint32_t*)(0x40064000U + 0x10U)) &= 0x0;
	tickCnt ++;
	tickCnt = tickCnt % 100;
	if (tickCnt == 4)	Tick1s();
	if (tickCnt % 50 == 3)	Tick500ms();
	if (tickCnt % 20 == 2)	Tick100ms();
	if (tickCnt % 2 == 1)	Tick20ms();
	(*(uint32_t*)(0x40064000U + 0x14U)) |= 0x2;	// clear flag
	(*(uint32_t*)(0x40064000U + 0x10U)) |= 0x2;		// Enable interrupt
}