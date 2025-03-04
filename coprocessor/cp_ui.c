#include "coprocessor/cp_ui.h"

uint8_t cp_menuItemNum = 5;
const char* cp_freqChangeStepDisp[] = {"100k", "10k ", "1k  ", "0.1k"};
static char String[32];
static char String2[32];

void UI_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool fill, bool bold)
{
	uint8_t i = 0;
	int8_t DeltaY = 0, DeltaX = 0;
	float k = 0, b = 0;
	if(x1 > x2)
	{
		i = x2;
		x2 = x1;
		x1 = i;
		i = y2;
		y2 = y1;
		y1 = i;
		i = 0;
	}
	DeltaY = y2 - y1;
	DeltaX = x2 - x1;
	if(DeltaX == 0)
	{
		if(y1 <= y2)
		{
			for(y1 = y1; y1<=y2; y1 ++)
			{
				PutPixel(x1, y1, fill);
				if (bold)
				{
					PutPixel(x1 + 1, y1, fill);
					PutPixel(x1 - 1, y1, fill);
					PutPixel(x1, y1 + 1, fill);
					PutPixel(x1, y1 - 1, fill);
				}
			}
		}
        else if(y1>y2)
		{
			for(y2 = y2; y2 <= y1; y2 ++)
			{
				PutPixel(x1, y2, fill);
				if (bold)
				{
					PutPixel(x1 + 1, y2, fill);
					PutPixel(x1 - 1, y2, fill);
					PutPixel(x1, y2 + 1, fill);
					PutPixel(x1, y2 - 1, fill);
				}
			}
		}
	}
	else if(DeltaY == 0)
	{
		for(x1 = x1; x1 <= x2; x1++)
		{
			PutPixel(x1, y1, fill);
			if (bold)
			{
				PutPixel(x1 + 1, y1, fill);
				PutPixel(x1 - 1, y1, fill);
				PutPixel(x1, y1 + 1, fill);
				PutPixel(x1, y1 - 1, fill);
			}
		}	
	}
	else
	{
		k = ((float)DeltaY) / ((float)DeltaX);
		b = y2 - k * x2;
		if(k > -1 && k < 1)
		{
			for(x1 = x1; x1 <= x2; x1 ++)
			{
				PutPixel(x1,(int)(k * x1 + b), fill);
				if (bold)
				{
					PutPixel(x1 + 1,(int)(k * x1 + b), fill);
					PutPixel(x1 - 1,(int)(k * x1 + b), fill);
					PutPixel(x1,(int)(k * x1 + b + 1), fill);
					PutPixel(x1,(int)(k * x1 + b - 1), fill);
				}
			}
		}
		else if((k >= 1) || (k <= -1))
		{
			if(y1 <= y2)
			{
				for(y1 = y1; y1 <= y2; y1 ++)
				{
					PutPixel((int)((y1 - b) / k), y1, fill);
					if (bold)
					{
						PutPixel((int)((y1 - b) / k + 1), y1, fill);
						PutPixel((int)((y1 - b) / k - 1), y1, fill);
						PutPixel((int)((y1 - b) / k), y1 + 1, fill);
						PutPixel((int)((y1 - b) / k), y1 - 1, fill);
					}
				}
			}
            else if(y1>y2)
			{
				for(y2 = y2; y2 <= y1; y2 ++)
				{
					PutPixel((int)((y2 - b) / k), y2, fill);
					if (bold)
					{
						PutPixel((int)((y2 - b) / k + 1), y2, fill);
						PutPixel((int)((y2 - b) / k - 1), y2, fill);
						PutPixel((int)((y2 - b) / k), y2 + 1, fill);
						PutPixel((int)((y2 - b) / k), y2 - 1, fill);
					}
				}
			}
		}
	}
}


void UI_DrawCircle(int centerx, int centery, int radius) 
{
    // X-axis multiplied by 1.4 because the pixel shape is not square
    // So radius represents the actual scale of the Y-axis
	for (int i = 1; i < 180; i ++) 
    {
		UI_DrawLine(centerx + radius * 1.4f * (sinLUT2deg100[i] / 100.0f), centery - radius * (cosLUT2deg100[i] / 100.0f), 
				centerx + radius * 1.4f * (sinLUT2deg100[i - 1] / 100.0f), centery - radius * (cosLUT2deg100[i - 1] / 100.0f),
				true, false);
	}
}

void CP_UI_DrawEmptyRadar(void)
{
    UI_DrawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, 15);
	UI_DrawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, 7);
	UI_DrawLine(RADAR_CENTER_X - 21, RADAR_CENTER_Y, RADAR_CENTER_X + 21, RADAR_CENTER_Y, true, false);
	UI_DrawLine(RADAR_CENTER_X, RADAR_CENTER_Y - 15, RADAR_CENTER_X, RADAR_CENTER_Y + 15, true, false);
}

uint8_t CP_UI_DrawRadar(st_satStatusMsgPack *satList, uint8_t selectedIndex, st_satPredict120min_2min pred, enum_satRadarDisplayMode displayMode)
{
	st_satStatusMsgPack selectedSat = satList[selectedIndex];
	int16_t az;
	int16_t el;
	int16_t satx, saty, satx1, saty1;
	int8_t i, j;
	uint8_t ret = 0;

	CP_UI_DrawEmptyRadar();

	if (displayMode == ALL)
	{
		
		for (i = 0; i < 10; i ++)
		{
			if (satList[i].valid != 1) continue;
			ret ++;
			az = satList[i].satAz_1Deg;
			el = abs(satList[i].satEl_1Deg);
			satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
			saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
			PutPixel(satx, saty, 1);
			PutPixel(satx + 1, saty, 1);
			PutPixel(satx - 1, saty, 1);
			PutPixel(satx, saty + 1, 1);
			PutPixel(satx, saty - 1, 1);
		}
	}
	else
	{
		if (selectedSat.valid != 1) return ret;

		if (displayMode == SINGLE_WITH_PREDICT)
		{
			for (i = 0; i < 59; i ++)
			{
				az = pred.az_2Deg[i];
				el = abs(pred.el_2Deg[i]);
				satx = RADAR_CENTER_X + ((45-el) / 3) * (sinLUT2deg100[az] * 1.4f / 100.0f);
				saty = RADAR_CENTER_Y - ((45-el) / 3) * (cosLUT2deg100[az] / 100.0f);
				az = pred.az_2Deg[i + 1];
				el = abs(pred.el_2Deg[i + 1]);
				satx1 = RADAR_CENTER_X + ((45-el) / 3) * (sinLUT2deg100[az] * 1.4f / 100.0f);
				saty1 = RADAR_CENTER_Y - ((45-el) / 3) * (cosLUT2deg100[az] / 100.0f);

				UI_DrawLine(satx, saty, satx1, saty1, true, pred.el_2Deg[i] > 0);
			}
		}
		
		az = selectedSat.satAz_1Deg;
		el = abs(selectedSat.satEl_1Deg);
		satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
		saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);

		for (i = -3; i < 4; i ++) 
		{
			for (j = -2; j < 3; j ++)
			{
				PutPixel(satx + i, saty + j, ((i > -3) && (i < 3) && (j > -2) && (j < 2))==(selectedSat.satEl_1Deg < 0));
			}
		}
		ret = 1;
	}
	return ret;
}

void CP_UI_DrawFrequency(uint32_t rx10hz, uint32_t tx10hz, enum_FreqChannel channel, enum_FrequencyTrackMode mode, bool isFreqInputMode, char* p_freqInputString, bool isTx)
{
    // Frequency unit in BK4819 is 10Hz
    sprintf(String, "%lu.%03lu %03lu", rx10hz / 100000, rx10hz % 100000 / 100, rx10hz % 100 * 10);
	sprintf(String2, "%lu.%03lu %03lu", tx10hz / 100000, tx10hz % 100000 / 100, tx10hz % 100 * 10);

	if (channel == CH_RX || channel == CH_RXMASTER)
	{
		UI_PrintStringSmallBold(isFreqInputMode ? p_freqInputString : String, 0, 127, 0);
		UI_PrintStringSmall(String2, 0, 127, 1);
	} else {
		UI_PrintStringSmall(String, 0, 127, 0);
		UI_PrintStringSmallBold(isFreqInputMode ? p_freqInputString : String2, 0, 127, 1);
	}

	if (isTx) 
	{
		for (int i = 0; i < 15; i ++) {
			gFrameBuffer[1][i] = 0b11111111;
		}
	}

	sprintf(String, "Rx");
	UI_PrintStringSmallest(String, 5, 1, false, 3);
	sprintf(String, "Tx");
	UI_PrintStringSmallest(String, 5, 9, false, !isTx);
}

void CP_UI_DrawQuickMenu(char* item1, char* item2, char* item3, uint8_t sel)
{
    for (int i = 0; i < 3; i ++) 
    {
		for (int j = 0; j < QUICKMENU_CELL_WIDTH; j ++) 
		{
			gFrameBuffer[3][j] = 0b10000000;
			gFrameBuffer[4 + i][j] = 0b10000000;
			if (sel == i+1) gFrameBuffer[4 + i][j] |= 0b01111111;
		}
		gFrameBuffer[4 + i][0] |= 0b11111111;
		gFrameBuffer[4 + i][QUICKMENU_CELL_WIDTH - 1] |= 0b11111111;
    }
    UI_PrintStringSmallest(item1, 2, 4 * 8 + 1, false, !(sel == 1));
    UI_PrintStringSmallest(item2, 2, 5 * 8 + 1, false, !(sel == 2));
    UI_PrintStringSmallest(item3, 2, 6 * 8 + 1, false, !(sel == 3));
}

void CP_UI_DrawRssi(int dbm, uint8_t s, double enable)
{
    for (int i = 0; i < 46; i ++) 
	{
		if (i % 5 == 0) 
		{
			gFrameBuffer[2][i + RSSI_METER_PADDING_LEFT] |= 0b01100000;
		} else {
			gFrameBuffer[2][i + RSSI_METER_PADDING_LEFT] |= 0b01000000;
		}
	}

    s = s >= 10 ? 10 : s;
	if (enable) 
	{
		sprintf(String, "%ddBm", dbm);
		UI_PrintStringSmallest(String, 0, 18, false, true);
		if (s < 10) sprintf(String, "S%u", s);
		else sprintf(String, "S9+");
		UI_PrintStringSmallest(String, 32, 18, false, true);

        for (int i = 0; i < s; i ++) 
		{
			for (int j = 0; j < 5; j ++) 
			{
				gFrameBuffer[2][i * 5 + j + RSSI_METER_PADDING_LEFT] |= 0b00011100;
			}
		}
	} else {
		sprintf(String, "------- --");
		UI_PrintStringSmallest(String, 0, 17, false, true);
	}
}

void CP_UI_DrawChannelIcon(enum_FreqChannel currentChannel) 
{
	if (currentChannel == CH_RX || currentChannel == CH_TXMASTER) 
    {		// Left arrow @ RX freq
		gFrameBuffer[0][98] = 0b00011000;
		gFrameBuffer[0][99] = 0b00111100;
		gFrameBuffer[0][100] = 0b01111110;
	}
    else if (currentChannel == CH_TX || currentChannel == CH_RXMASTER) 
    {		// Left arrow @ Tx freq
		gFrameBuffer[1][98] = 0b00011000;
		gFrameBuffer[1][99] = 0b00111100;
		gFrameBuffer[1][100] = 0b01111110;
	}
    if (currentChannel == CH_RXMASTER || currentChannel == CH_TXMASTER) 
    {		// Link icon
		for (int i = 0; i < 2; i ++) 
        {
			for (int j = 0; j < 6; j ++) 
            {
				gFrameBuffer[i][98 + j] |= 0b00011000;
			}
		}
		gFrameBuffer[0][103] |= 0b11111000;
		gFrameBuffer[0][104] |= 0b11111000;
		gFrameBuffer[1][103] |= 0b00011111;
		gFrameBuffer[1][104] |= 0b00011111;
	}
}


void CP_UI_DrawStatusBarInMainScreen(st_time24MsgPack time, uint8_t battLevel, bool gnss, uint8_t freqStep, ModulationType modulation, BK4819_FilterBandwidth_t bw)
{
	gStatusLine[127] = 0b00111111;
	for (int i = 126; i >= 116; i--) 
	{
		gStatusLine[i] = 0b00100001;
	}
	battLevel <<= 1;
	for (int i = 125; i >= 116; i--) 
	{
		if (126 - i <= battLevel) 
		{
			gStatusLine[i + 2] = 0b00111111;
		}
	}
	gStatusLine[117] = 0b00111111;
	gStatusLine[116] = 0b00001100;

	sprintf(String, "%02d:%02d:%02d", 
			time.hh, time.mm, time.ss);
	UI_PrintStringSmallest(String, 0, 0, true, true);

	if (gnss) UI_PrintStringSmallest("G", 40, 0, true, true);

	sprintf(String, "%s %s",
		modulationTypeOptions[modulation],
		bwNames[bw]);
	UI_PrintStringSmallest(String, 72, 0, true, true);

	if (freqStep > 4) return;

	uint8_t stepIndcStart = freqStep == 3 ? 76 : (48 + freqStep * 7);
	for (int i = 0; i < 6; i ++) 
	{
		gStatusLine[i + stepIndcStart] |= 0b01000000;
		gFrameBuffer[2][i + stepIndcStart] |= 0b00000001;
	}
}

void CP_UI_DrawStatusBarInMenu(st_time24MsgPack time, uint8_t battLevel, bool gnss)
{
	gStatusLine[127] = 0b00111111;
	for (int i = 126; i >= 116; i--) 
	{
		gStatusLine[i] = 0b00100001;
	}
	battLevel <<= 1;
	for (int i = 125; i >= 116; i--) 
	{
		if (126 - i <= battLevel) 
		{
			gStatusLine[i + 2] = 0b00111111;
		}
	}
	gStatusLine[117] = 0b00111111;
	gStatusLine[116] = 0b00001100;

	sprintf(String, "%02d:%02d:%02d", 
			time.hh, time.mm, time.ss);
	UI_PrintStringSmallest(String, 0, 0, true, true);

	if (gnss) UI_PrintStringSmallest("G", 40, 0, true, true);
}

void CP_UI_SubMenu_DrawSingleSatInfo(st_satStatusMsgPack *satInfoList, uint8_t selectedSatIdx)
{
	if (satInfoList[selectedSatIdx].valid == 0)
	{
		UI_PrintStringSmallest("NO DATA", 78, 8, false, true);
		return;
	}
	
	sprintf(String, "A:%03d E:%03d", 
		satInfoList[selectedSatIdx].satAz_1Deg, 
		satInfoList[selectedSatIdx].satEl_1Deg);
	UI_PrintStringSmallest(String, 78, 0, false, true);

	sprintf(String, "U:%09d", 
		(satInfoList[selectedSatIdx].uplinkFreq + satInfoList[selectedSatIdx].uplinkDoppler));
	UI_PrintStringSmallest(String, 78, 8, false, true);

	sprintf(String, "D:%09d", 
		(satInfoList[selectedSatIdx].downlinkFreq - satInfoList[selectedSatIdx].downlinkDoppler));
	UI_PrintStringSmallest(String, 78, 16, false, true);
}

void CP_UI_SubMenu_DrawGnssInfo(st_time24MsgPack *time, st_siteInfoMsgPack *site, bool fixed)
{
	if (!fixed) UI_PrintStringSmallest("LOCATING", 78, 8, false, true);
	else UI_PrintStringSmallest("FIXED", 78, 8, false, true);

	sprintf(String, "%02d:%02d:%02d", time->hh, time->mm, time->ss);
	UI_PrintStringSmallest(String, 78, 16, false, true);

	sprintf(String, "%03d.%03d%c", site->siteLat_Deg, site->siteLat_0_001Deg, site->NS);
	UI_PrintStringSmallest(String, 78, 24, false, true);

	sprintf(String, "%03d.%03d%c", site->siteLon_Deg, site->siteLon_0_001Deg, site->EW);
	UI_PrintStringSmallest(String, 78, 32, false, true);

	UI_PrintStringSmallest(site->siteMaidenhead, 78, 40, false, true);

}

void CP_UI_SubMenu_DrawGnssInfo_Detail(st_time24MsgPack *time, st_siteInfoMsgPack *site, char* gnrmc, int8_t tz, bool fixed)
{
	UI_PrintStringSmallBold("GNSS INFO", 0, 127, 0);
	if (tz >= 0) sprintf(String, "%02d:%02d:%02d GMT+%02d", time->hh, time->mm, time->ss, tz);
	else sprintf(String, "%02d:%02d:%02d GMT-%02d", time->hh, time->mm, time->ss, -tz);
	UI_PrintStringSmall(String, 8, 128, 1);

	if (fixed) UI_PrintStringSmallBold("LOCATION FOUND", 0, 127, 2);
	else UI_PrintStringSmall("LOCATING", 0, 127, 2);

	sprintf(String, "%03d.%03d%c %03d.%03d%c %s", 
		site->siteLat_Deg, site->siteLat_0_001Deg, site->NS,
		site->siteLon_Deg, site->siteLon_0_001Deg, site->EW,
		site->siteMaidenhead);
	UI_PrintStringSmallest(String, 10, 24, false, true);

	UI_PrintStringSmallest(gnrmc, 0, 32, false, true);
}

void CP_UI_SubMenu_DrawTleInfo(char* tleString, uint8_t slot, bool tleValid, bool isDeleteMode)
{
	UI_PrintStringSmallBold("TLE IMPORTER", 0, 127, 0);
	if (isDeleteMode) sprintf(String, "DELETE>>SLOT%02d", slot);
	else
	{
		if (!tleValid) sprintf(String, "INVALID>>SLOT%02d", slot);
		else sprintf(String, "VALID>>SLOT%02d", slot);
	}
	UI_PrintStringSmall(String, 0, 127, 1);

	UI_PrintStringSmallest(tleString, 0, 24, false, true);
}

void CP_UI_DrawListFrame()
{
	for (int i = 0; i < 7; i++) {
		gFrameBuffer[i][75] = 0xFF;
	}
	for (int i = 0; i < 75; i++) {
		gFrameBuffer[0][i] |= 0b10000000;
	}
}

void CP_UI_Menu_DrawMenuList(char *title, uint8_t menuSelectIdx, char **itemList, uint8_t itemCount)
{
	uint8_t offset = 0;
	if (itemCount >= 5) offset = Clamp(menuSelectIdx - 2, 0, itemCount - 6);
	UI_PrintStringSmallBold(title, 0, 80, 0);
	for (int i = 0; i < Clamp(itemCount, 1, 6); i ++) 
	{
		sprintf(String, "%s", itemList[i + offset]);
		String[10] = 0;
		
		if (menuSelectIdx == (i + offset)) {
			UI_PrintStringSmallBold(String, 0, 80, i + 1);
		} else {
			UI_PrintStringSmall(String, 0, 80, i + 1);
		}
	}
	CP_UI_DrawListFrame();
}

void CP_UI_Menu_DrawSatList(char *title, st_satStatusMsgPack *satInfoList, uint8_t selectedSatIdx, bool enableDoppler, uint8_t dopplerIdx)
{
	uint8_t offset = Clamp(selectedSatIdx - 2, 0, 10 - 6);
	UI_PrintStringSmallBold(title, 0, 80, 0);
	for (int i = 0; i < 6; i ++) 
	{
		sprintf(String, "%s", satInfoList[i + offset].valid ? satInfoList[i + offset].name : "---");
		String[10] = 0;
		
		if (selectedSatIdx == (i + offset)) {
			UI_PrintStringSmallBold(String, 0, 80, i + 1);
		} else {
			UI_PrintStringSmall(String, 0, 80, i + 1);
		}

		if (enableDoppler && dopplerIdx == (i + offset)) UI_PrintStringSmall(">", 0, 0, i + 1);
	}
	CP_UI_DrawListFrame();
}