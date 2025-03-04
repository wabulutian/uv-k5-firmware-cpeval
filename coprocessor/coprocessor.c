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


static uint8_t scanRssi;

static uint8_t txRestoreTime = 2;
static uint8_t tickCnt = 0;

static KEY_Code_t btn;
static uint8_t btnPrev;
static uint8_t btnDownCounter;

static uint32_t cp_tempFreq;
static uint8_t cp_freqInputIndex = 0;
static KEY_Code_t cp_freqInputArr[8];
static char cp_freqInputString[13] = "---.--- --0\0";

static st_satStatusMsgPack satLiveList[10];
static st_satPredict120min_2min focusedSatPred;
static char satNameList[10][11];
st_time24MsgPack localTime;
static st_siteInfoMsgPack siteInfo;
static char gnrmc[75];
static uint8_t gnssFixed = 0;
static int8_t tzSet = 0;
static enum_SubMenu cp_subMenuState = GNSS;
static int8_t cp_subMenuSelectIdx = 0;
static uint8_t cp_satDataRequestSlice = 0;
static bool cp_requestAllSatData = false;
static bool cp_i2cReqSetTz = false;
static uint8_t cp_i2cReqNewTle = 0;
static bool cp_i2cReqAddSat = false;
static bool cp_i2cReqDelSat = false;
static bool cp_listenUartData = false;
static uint8_t cp_tleDisplayLine = 0;
static bool cp_tleValid = false;
static bool cp_isTleDeleteMode = false;
static char cp_tle_line[5][71];
static bool dopplerTuneReq = false;
uint8_t cp_menuSelectIdx = 0;

static bool cp_enableDoppler = false;
static uint8_t cp_selectedSatIdx = 0;
static un_Freq dopplerBackup = {0};
static uint8_t dopplerTuneTick = 0;

st_CpSettings cp_settings = 
{
	.currentChannel = CH_RX,
	.txMode = TX_FM,
	.ctcssToneIdx = 0,
	.freqTrackMode = FREQTRACK_OFF,
    .transponderType = TRANSPONDER_NORMAL,
};

const char* cp_freqTrackModeOptionDisp[] = {"OFF", "FM", "LINE"};
const char* cp_transponderTypeOptionDisp[] = {"NORMAL", "INVERSE"};
const char* cp_menuItemList[CP_MENU_ITEM_NUMBER] = {"GNSS", "SAT", "TLE"};
const uint8_t cp_subMenuItemNumList[CP_MENU_ITEM_NUMBER] = {0, 10, 10};

#pragma region <<Utils>>

static bool IsFreqLegal(uint32_t f)
{
	// if (f > 13000000 && f < 15000000) return true;
	// if (f > 40000000 && f < 50000000) return true;
	// return false;
	return true;
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

#pragma endregion

#pragma region <<UI Drawing>>

char String[32];
static void CP_UI_DrawMainInterface()
{
	CP_UI_DrawFrequency(mainFreq.data.down, mainFreq.data.up, cp_settings.currentChannel, cp_settings.freqTrackMode, cp_currentState == FREQ_INPUT, cp_freqInputString, isTransmitting);
	CP_UI_DrawChannelIcon(cp_settings.currentChannel);

	int dbm = Rssi2DBm(scanRssi);	// in measurements.h by @Fagci
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
	else
	{
		sprintf(String, "-----");
	}
	CP_UI_DrawQuickMenu(cp_freqTrackModeOptionDisp[cp_settings.freqTrackMode],
						cp_transponderTypeOptionDisp[cp_settings.transponderType],	
						String,
						cp_quickMenuState);

	if (!cp_enableDoppler) CP_UI_DrawEmptyRadar();
	else
	{
		UI_PrintStringSmallest(satLiveList[cp_selectedSatIdx].name, 34, 4 * 8 + 1, false, true);
		sprintf(String, "A:%03d E:%03d", 
			satLiveList[cp_selectedSatIdx].satAz_1Deg, 
			satLiveList[cp_selectedSatIdx].satEl_1Deg);
		UI_PrintStringSmallest(String, 34, 5 * 8 + 1, false, true);
		sprintf(String, "%03d:%02d",  focusedSatPred.nextEvent_mm, focusedSatPred.nextEvent_ss);
		UI_PrintStringSmallest(String, 34, 6 * 8 + 1, false, true);
		CP_UI_DrawRadar(&satLiveList, cp_selectedSatIdx, focusedSatPred, cp_enableDoppler ? SINGLE_WITH_PREDICT : SINGLE);
	}

	UpdateBatteryInfo();
	CP_UI_DrawStatusBarInMainScreen(localTime, gBatteryDisplayLevel, gnssFixed, cp_currentFreqChangeStepIdx, cp_modulationType, listenBw);

	ST7565_BlitFullScreen();
	ST7565_BlitStatusLine();
}

static void CP_UI_DrawMenuInterface()
{
	UpdateBatteryInfo();

	CP_UI_DrawStatusBarInMenu(localTime, gBatteryDisplayLevel, gnssFixed);
	CP_UI_Menu_DrawMenuList("MAIN MENU", cp_menuSelectIdx, cp_menuItemList, CP_MENU_ITEM_NUMBER);
	switch (cp_menuSelectIdx)
	{
	case GNSS:
		CP_UI_SubMenu_DrawGnssInfo(&localTime, &siteInfo, gnssFixed);
		break;
	case SAT_INFO:
		sprintf(String, "Valid Sat:%02d", CP_UI_DrawRadar(&satLiveList, cp_subMenuSelectIdx, focusedSatPred, ALL));
		UI_PrintStringSmallest(String, 78, 8, false, true);
		break;
	case IMPORT_TLE:
		break;
	default:
		break;
	}

	ST7565_BlitFullScreen();
	ST7565_BlitStatusLine();
}

static void CP_UI_DrawSubMenuInterface()
{
	UpdateBatteryInfo();
	CP_UI_DrawStatusBarInMenu(localTime, gBatteryDisplayLevel, gnssFixed);
	
	switch (cp_subMenuState)
	{
	case GNSS:
		CP_UI_SubMenu_DrawGnssInfo_Detail(&localTime, &siteInfo, gnrmc, tzSet, gnssFixed);
		break;
	case SAT_INFO:
		CP_UI_DrawRadar(&satLiveList, cp_subMenuSelectIdx, focusedSatPred, cp_enableDoppler ? SINGLE_WITH_PREDICT : SINGLE);
		CP_UI_SubMenu_DrawSingleSatInfo(&satLiveList, cp_subMenuSelectIdx);
		CP_UI_Menu_DrawSatList("SAT LIVE", &satLiveList, cp_subMenuSelectIdx, cp_enableDoppler, cp_selectedSatIdx);
		break;
	case IMPORT_TLE:
		CP_UI_SubMenu_DrawTleInfo(cp_tle_line[cp_tleDisplayLine], cp_subMenuSelectIdx, cp_tleValid, cp_isTleDeleteMode);
		break;
	default:
		break;
	}
	

	ST7565_BlitFullScreen();
	ST7565_BlitStatusLine();
}


#pragma endregion

#pragma region <<TLE>>


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

	if (cp_settings.currentChannel == CH_RXMASTER && cp_settings.freqTrackMode == FREQTRACK_OFF) 
	{
		mainFreq.data.up = f;
	}
	else if (cp_settings.currentChannel == CH_TXMASTER && cp_settings.freqTrackMode == FREQTRACK_OFF) 
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

	if (cp_settings.currentChannel == CH_RXMASTER && cp_settings.freqTrackMode == FREQTRACK_OFF) 
	{
		mainFreq.data.up = f;
	}
	else if (cp_settings.currentChannel == CH_TXMASTER && cp_settings.freqTrackMode == FREQTRACK_OFF) 
	{
		BK4819_TuneTo(f);
		mainFreq.data.down = f;
	}

	if (cp_settings.freqTrackMode != FREQTRACK_OFF) dopplerTuneReq = true;
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
		cp_freqInputArr[cp_freqInputIndex] = key;
		cp_freqInputIndex++;
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
			cp_settings.freqTrackMode == FREQTRACK_LINEAR ? FREQTRACK_OFF : (cp_settings.freqTrackMode + 1);
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
		break;
	default:
		break;
	}
}
#pragma endregion

#pragma region <<USER_INPUT>>

/**
 *  Key value reading functions from @Fagci
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
		if (cp_quickMenuState == MENU_NONE && !(cp_enableDoppler && cp_settings.freqTrackMode == FREQTRACK_FM)) CP_FreqInput();
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
		if (!(cp_enableDoppler && cp_settings.freqTrackMode == FREQTRACK_FM)) UpdateCurrentFreqNormal(true);
		break;
	case KEY_DOWN:
		if (cp_quickMenuState != MENU_NONE) 
		{
			CP_ToggleQuickMenuValue(cp_quickMenuState, false);
			break;
		}
		if (!(cp_enableDoppler && cp_settings.freqTrackMode == FREQTRACK_FM)) UpdateCurrentFreqNormal(false);
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
		// ToggleBacklight();
		break;
	case KEY_PTT:
		if (!IsFreqLegal(mainFreq.data.up)) break;
		if (cp_settings.txMode == TX_FM && cp_modulationType == MOD_FM) ToggleTX(true);
		break;
	case KEY_MENU:
		preventKeyDown = true;
		if (cp_quickMenuState != MENU_NONE) cp_quickMenuState = MENU_NONE;
		else CP_SetState(MENU);
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
		if (cp_menuSelectIdx == 0) cp_menuSelectIdx = CP_MENU_ITEM_NUMBER-1;
		else cp_menuSelectIdx --;
	break;
	case KEY_DOWN:
		cp_menuSelectIdx ++;
		cp_menuSelectIdx = cp_menuSelectIdx % CP_MENU_ITEM_NUMBER;
	break;
	case KEY_EXIT:
		CP_SetState(NORMAL);
		break;
	case KEY_MENU:
		cp_subMenuState = cp_menuSelectIdx;
		if (cp_subMenuState == GNSS && !cp_enableDoppler) tzSet = localTime.tz;	// dont change tle while doppler enabled
		else if (cp_subMenuState == IMPORT_TLE) cp_listenUartData = true;
		CP_SetState(SUBMENU);
		break;
	default:
		break;
	}
	preventKeyDown = true;
}

static void OnKeyDownSubMenu(uint8_t key) 
{
	switch (key) 
	{
	case KEY_0: break;
	case KEY_1:
		cp_tleDisplayLine = 0;
	break;
	case KEY_2:
		cp_tleDisplayLine = 1;
	break;
	case KEY_3:
		cp_tleDisplayLine = 2;
	break;
	case KEY_4:
		cp_tleDisplayLine = 3;
	break;
	case KEY_5:
		cp_tleDisplayLine = 4;
	break;
	case KEY_6: break;
	case KEY_7: break;
	case KEY_8: break;
	case KEY_9: break;
	case KEY_STAR: 
		if (cp_subMenuState == IMPORT_TLE) 
		{
			cp_listenUartData = false;
			cp_subMenuState = SAT_INFO;
		}
		else if (cp_subMenuState == SAT_INFO && !cp_enableDoppler)
		{
			cp_listenUartData = true;
			cp_subMenuState = IMPORT_TLE;
		}
	break;
	case KEY_F:
		if (cp_subMenuState == IMPORT_TLE) 
		{
			cp_isTleDeleteMode = !cp_isTleDeleteMode;
		}
	break;
	case KEY_UP:
		// cant change selected satellite while doppler enabled
		if (cp_subMenuState == SAT_INFO && cp_enableDoppler) break;
		if (cp_subMenuState == GNSS)
		{
			tzSet = tzSet == 12 ? -12 : (tzSet + 1);
		}
		else
		{
			if (cp_subMenuState == IMPORT_TLE) cp_subMenuSelectIdx ++;
			else  cp_subMenuSelectIdx --;
			if (cp_subMenuSelectIdx < 0) cp_subMenuSelectIdx = cp_subMenuItemNumList[cp_subMenuState] - 1;
			cp_subMenuSelectIdx = cp_subMenuSelectIdx % cp_subMenuItemNumList[cp_subMenuState];
		}
	break;
	case KEY_DOWN:
		// cant change selected satellite while doppler enabled
		if (cp_subMenuState == SAT_INFO && cp_enableDoppler) break;
		if (cp_subMenuState == GNSS)
		{
			tzSet = tzSet == -12 ? 12 : (tzSet - 1);
		}
		else
		{
			if (cp_subMenuState == IMPORT_TLE) cp_subMenuSelectIdx --;
			else  cp_subMenuSelectIdx ++;
			if (cp_subMenuSelectIdx < 0) cp_subMenuSelectIdx = cp_subMenuItemNumList[cp_subMenuState] - 1;
			cp_subMenuSelectIdx = cp_subMenuSelectIdx % cp_subMenuItemNumList[cp_subMenuState];
		}
	break;
	case KEY_EXIT:
		cp_listenUartData = false;
		CP_SetState(cp_previousState);
	break;
	case KEY_MENU:
		if (cp_subMenuState == GNSS) cp_i2cReqSetTz = true;
		else if (cp_subMenuState == IMPORT_TLE && (cp_tleValid || cp_isTleDeleteMode))
		{
			if (cp_isTleDeleteMode) cp_i2cReqDelSat = true;
			else cp_i2cReqAddSat = true;
			cp_listenUartData = false;
			CP_SetState(cp_previousState);
		}
		else if (cp_subMenuState == SAT_INFO)
		{
			if (!cp_enableDoppler)
			{
				cp_enableDoppler = true;
				cp_selectedSatIdx = cp_subMenuSelectIdx;
			}
			else
			{
				cp_enableDoppler = false;
			}
		}
	break;
	default:
		break;
	}
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
		case SUBMENU:
			OnKeyDownSubMenu(btn);
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
}

static void Tick200ms()
{
	Measure();
	CP_I2C_Write(0x0012, &cp_enableDoppler, 1);
	CP_I2C_Write(0x0013, &cp_selectedSatIdx, 1);
	if (cp_i2cReqSetTz)
	{
		CP_I2C_Write(0x0080, &tzSet, 1);
		cp_i2cReqSetTz = false;
	}
	if (cp_i2cReqAddSat)
	{
		CP_I2C_Write(0x0010, &cp_subMenuSelectIdx, 1);
		cp_i2cReqAddSat = false;
	}
	if (cp_i2cReqDelSat)
	{
		CP_I2C_Write(0x0011, &cp_subMenuSelectIdx, 1);
		cp_i2cReqDelSat = false;
	}
	if (cp_listenUartData) 
	{
		// Check RXTO bit to confirm whether UART is idle
		if (((UART1->IF & UART_IF_RXTO_MASK) >> UART_IF_RXTO_SHIFT) == 0x1)
		{
			// Clear RXTO bit
			UART1->IF |= UART_IF_RXTO_BITS_SET;
			// Process rx data
			SProto_UART_to_I2C_Passthrough();
		}
	}
	CP_I2C_Read(0x0000, &siteInfo, 18);
	CP_I2C_Read(0x0001, &localTime, 8);
	CP_I2C_Read(0x0003, gnrmc, 75);
	CP_I2C_Read(0x0004, &gnssFixed, 1);
	CP_I2C_Read(0x0083, &cp_tleValid, 1);

	if (cp_enableDoppler)
	{
		CP_I2C_Read(0x0200 + cp_selectedSatIdx, &focusedSatPred, 124);
	}
	if (cp_currentState == SUBMENU && cp_subMenuState == IMPORT_TLE && cp_i2cReqNewTle == 1)
	{
		CP_I2C_Read(0x0082, cp_tle_line, 5*71);
		cp_i2cReqNewTle = 2;
	}
	if (cp_requestAllSatData)
	{
		memset(satLiveList[cp_satDataRequestSlice].name, 0, 11);
		CP_I2C_Read(0x0100 + cp_satDataRequestSlice, &satLiveList[cp_satDataRequestSlice], 30);
		if (satLiveList[cp_satDataRequestSlice].valid) memcpy(satNameList[cp_satDataRequestSlice], satLiveList[cp_satDataRequestSlice].name, 11);
		else sprintf(satNameList[cp_satDataRequestSlice], "---");
		cp_satDataRequestSlice += 1;
		cp_satDataRequestSlice = cp_satDataRequestSlice % 10;
	}
	else
	{
		memset(satLiveList[cp_subMenuSelectIdx].name, 0, 11);
		CP_I2C_Read(0x0100 + cp_subMenuSelectIdx, &satLiveList[cp_subMenuSelectIdx], 30);
		if (satLiveList[cp_subMenuSelectIdx].valid) memcpy(satNameList[cp_subMenuSelectIdx], satLiveList[cp_subMenuSelectIdx].name, 11);
		else sprintf(satNameList[cp_subMenuSelectIdx], "---");
	}
}

static void Tick1s()
{
	if (dopplerTuneTick < 5)
	{
		dopplerTuneTick ++;
		return;
	}
	else
	{
		dopplerTuneTick = 0;
		BK4819_TuneTo(mainFreq.data.down);
	}
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

	// Init local freq
	if (isFirstBoot)
	{
		mainFreq.data.down = mainFreq.data.up = initialFreq =
			gEeprom.VfoInfo[gEeprom.TX_CHANNEL].pRX->Frequency;

		dopplerBackup.data.up = mainFreq.data.up;
		dopplerBackup.data.down = mainFreq.data.down;
		isFirstBoot = false;
	}
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
	SYSTEM_DelayMs(200);
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
	memset(gStatusLine, 0, sizeof(gStatusLine));
	// sprintf(String, "ADDR 0001020304050607 TEXT");
	// UI_PrintStringSmallest(String, 0, 0, true, true);
	ST7565_BlitStatusLine();
	preventKeyDown = false;
	while (isInitialized)
	{
		memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
		memset(gStatusLine, 0, sizeof(gStatusLine));
		if (cp_currentState == MENU)
		{
			cp_requestAllSatData = !cp_enableDoppler;
			CP_UI_DrawMenuInterface();
		}
		else if (cp_currentState == SUBMENU)
		{
			cp_requestAllSatData = false;
			CP_UI_DrawSubMenuInterface();
		}
		else
		{
			cp_requestAllSatData = false;
			CP_UI_DrawMainInterface();
		}

		if (!cp_tleValid)
		{
			cp_i2cReqNewTle = 0;
		}
		else if (cp_i2cReqNewTle == 0)
		{
			cp_i2cReqNewTle = 1;
		}
		if (!cp_enableDoppler)
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

		if (cp_enableDoppler && !isTransmitting)
		{
			if (cp_settings.freqTrackMode == FREQTRACK_FM)
			{
				mainFreq.data.up = (satLiveList[cp_selectedSatIdx].uplinkFreq + satLiveList[cp_selectedSatIdx].uplinkDoppler) / 10;
				mainFreq.data.down = (satLiveList[cp_selectedSatIdx].downlinkFreq - satLiveList[cp_selectedSatIdx].downlinkDoppler) / 10;
			}
			else if(cp_settings.freqTrackMode == FREQTRACK_LINEAR)
			{
				uint32_t uplinkCenter = (satLiveList[cp_selectedSatIdx].uplinkFreq + satLiveList[cp_selectedSatIdx].uplinkDoppler) / 10;
				uint32_t downlinkCenter = (satLiveList[cp_selectedSatIdx].downlinkFreq - satLiveList[cp_selectedSatIdx].downlinkDoppler) / 10;
				int32_t diff;
				if (cp_settings.currentChannel == CH_TXMASTER)
				{
					diff = mainFreq.data.up - uplinkCenter;
					if (cp_settings.transponderType == NORMAL) mainFreq.data.down = downlinkCenter + diff;
					else mainFreq.data.down = downlinkCenter - diff;
				}
				else if (cp_settings.currentChannel == CH_RXMASTER)
				{
					diff = mainFreq.data.down - downlinkCenter;
					if (cp_settings.transponderType == NORMAL) mainFreq.data.up = uplinkCenter + diff;
					else mainFreq.data.up = uplinkCenter - diff;
				}
			}
			if (dopplerTuneReq)
			{
				BK4819_TuneTo(mainFreq.data.down);
				dopplerTuneReq = false;
			}
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
	if (tickCnt % 20 == 2)	Tick200ms();
	if (tickCnt % 2 == 1)	Tick20ms();
	(*(uint32_t*)(0x40064000U + 0x14U)) |= 0x2;	// clear flag
	(*(uint32_t*)(0x40064000U + 0x10U)) |= 0x2;		// Enable interrupt
}