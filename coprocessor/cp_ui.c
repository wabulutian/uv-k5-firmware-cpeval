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
#ifdef ENABLE_PASS
void CP_UI_DrawRadar(un_SatInfo* list, uint8_t index, st_PassInfo pass, uint8_t mode) 
{
	st_SatInfo current = list[index].data;
    int16_t az;
    int8_t el;
    int16_t satx, saty, satx1, saty1;
	int8_t i, j;

	CP_UI_DrawEmptyRadar();

	if (mode == 2)
	{
		if (pass.active != 1) return;
		// Pass
		for (i = 0; i < 60; i ++)
		{
			az = pass.azElList[i].azimuth / 10;
			if (pass.azElList[i].elevation <= 0 && pass.azElList[i + 1].elevation <= 0)
			{
				el = -pass.azElList[i].elevation / 10.0f;
				satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
				saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
				PutPixel(satx, saty, 1);
			}
			else
			{
				az = pass.azElList[i].azimuth / 10;
				el = pass.azElList[i].elevation / 10;
				satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
				saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
				az = pass.azElList[i + 1].azimuth / 10;
				el = pass.azElList[i + 1].elevation / 10;
				satx1 = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
				saty1 = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
				UI_DrawLine(satx, saty, satx1, saty1, true, true);
			}
		}
	}

	if (mode == 1)
	{
		// List
		for (i = 0; i < 10; i ++)
		{
			if (list[i].data.valid != 1) continue;
			
			az = list[i].data.currentAz / 10;
			el = (list[i].data.currentEl < 0 ? (-list[i].data.currentEl / 10) : (list[i].data.currentEl / 10));
			satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
			saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
			PutPixel(satx, saty, 1);
			PutPixel(satx + 1, saty, 1);
			PutPixel(satx - 1, saty, 1);
			PutPixel(satx, saty + 1, 1);
			PutPixel(satx, saty - 1, 1);
		}
	}

	if (current.valid != 1) return;
    // Current position
    az = current.currentAz / 10;
    el = (current.currentEl < 0 ? (-current.currentEl / 10) : (current.currentEl / 10));
	satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
	saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
	for (i = -3; i < 4; i ++) 
    {
		for (j = -2; j < 3; j ++)
		{
			PutPixel(satx + i, saty + j, ((i > -3) && (i < 3) && (j > -2) && (j < 2))==(current.currentEl<0));
		}
	}
}
#else
void CP_UI_DrawRadar(un_SatInfo* list, uint8_t index, uint8_t mode) 
{
	st_SatInfo current = list[index].data;
    int16_t az;
    int8_t el;
    int16_t satx, saty, satx1, saty1;
	int8_t i, j;

	CP_UI_DrawEmptyRadar();

	if (mode == 1 || mode == 3)
	{
		// List
		for (i = 0; i < 10; i ++)
		{
			if (list[i].data.valid != 1) continue;
			
			az = list[i].data.currentAz / 10;
			el = (list[i].data.currentEl < 0 ? (-list[i].data.currentEl / 10) : (list[i].data.currentEl / 10));
			satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
			saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
			PutPixel(satx, saty, 1);
			PutPixel(satx + 1, saty, 1);
			PutPixel(satx - 1, saty, 1);
			PutPixel(satx, saty + 1, 1);
			PutPixel(satx, saty - 1, 1);
		}
	}

	if (current.valid != 1) return;
	if (mode == 3) return;
    // Current position
    az = current.currentAz / 10;
    el = (current.currentEl < 0 ? (-current.currentEl / 10) : (current.currentEl / 10));
	satx = RADAR_CENTER_X + ((90-el) / 6) * (sinLUT2deg100[az / 2] * 1.4f / 100.0f);
	saty = RADAR_CENTER_Y - ((90-el) / 6) * (cosLUT2deg100[az / 2] / 100.0f);
	for (i = -3; i < 4; i ++) 
    {
		for (j = -2; j < 3; j ++)
		{
			PutPixel(satx + i, saty + j, ((i > -3) && (i < 3) && (j > -2) && (j < 2))==(current.currentEl<0));
		}
	}
}
#endif

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
#ifdef ENABLE_PASS
void CP_UI_DrawSatInfo(st_SatInfo src, st_PassInfo pass)
{
	if (src.valid != 1 || pass.active != 1) return;
	sprintf(String, "%s", src.name);
	UI_PrintStringSmallest(String, 35, 33, false, true);

	sprintf(String, "%03d | %02d", src.currentAz / 10, src.currentEl / 10);
	UI_PrintStringSmallest(String, 35, 41, false, true);

	if (pass.aosMin > 180) sprintf(String2, "Next >3:00");
	else if (pass.aosMin == 0) sprintf(String2, "Next <0:01");
	else sprintf(String2, "Next %d:%02d", pass.aosMin / 60, pass.aosMin % 60);
	UI_PrintStringSmallest(String2, 35, 49, false, true);
}
#else
void CP_UI_DrawSatInfo(st_SatInfo src)
{
	if (!src.valid) return;

	sprintf(String, "%s", src.name);
	UI_PrintStringSmallest(String, 35, 33, false, true);

	sprintf(String, "%03d | %02d", src.currentAz / 10, src.currentEl / 10);
	UI_PrintStringSmallest(String, 35, 41, false, true);

	sprintf(String, "dV: %d", src.currentSpd);
	UI_PrintStringSmallest(String, 35, 49, false, true);
}
#endif

void CP_UI_DrawStatusBar(uint8_t freqStep, ModulationType modulation, BK4819_FilterBandwidth_t bw, uint8_t battLevel, st_Time time)
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
			time.hour, time.min, time.sec);
	UI_PrintStringSmallest(String, 0, 0, true, true);
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

void CP_UI_Menu_DrawSatList(uint8_t menuSelectIdx, un_SatInfo* satInfoList, bool enable, uint8_t selectedIdx)
{
	uint8_t offset = Clamp(menuSelectIdx - 2, 0, 11 - 6);
	for (int i = 0; i < 6; ++i) {

		if (i + offset == 0) sprintf(String, "DISABLE");
		else if (satInfoList[i + offset - 1].data.valid != 1) sprintf(String, "NO DATA");
		else sprintf(String, "%s", satInfoList[i + offset - 1].data.name);
		String[10] = 0;
		
		if (menuSelectIdx == (i + offset)) {
			UI_PrintStringSmallBold(String, 0, 80, i);
		} else {
			UI_PrintStringSmall(String, 0, 80, i);
		}

		if ((selectedIdx * enable) == (i + offset))
		{
			gFrameBuffer[i][72] = 0b00011000;
			gFrameBuffer[i][73] = 0b00111100;
			gFrameBuffer[i][74] = 0b01111110;
		}
	}

	for (int i = 0; i < 6; i++) {
		gFrameBuffer[i][75] = 0xFF;
	}
	for (int i = 0; i < 75; i++) {
		gFrameBuffer[5][i] |= 0b10000000;
	}
}

#ifdef ENABLE_PASS
void CP_UI_Menu_DrawSatInfo(st_SatInfo src, st_PassInfo pass)
{
	if (src.valid != 1 || pass.active != 1) return;

	sprintf(String, "%s", src.name);
	UI_PrintStringSmallest(String, 77, 0, false, true);
	sprintf(String, "%03d | %02d", src.currentAz / 10, src.currentEl / 10);
	UI_PrintStringSmallest(String, 77, 8, false, true);
	sprintf(String, "%03d.%03d | %03d.%03d", 
		src.uplinkFreq / 100000, (src.uplinkFreq / 100) % 1000,
		src.downlinkFreq / 100000, (src.downlinkFreq / 100) % 1000);
	UI_PrintStringSmallest(String, 2, 49, false, true);

	if (pass.aosMin > 180) sprintf(String, ">3:00");
	else if (pass.aosMin == 0) sprintf(String, "<0:01");
	else sprintf(String, "%d:%02d", pass.aosMin / 60, pass.aosMin % 60);
	UI_PrintStringSmallest(String, 77, 16, false, true);
}
#else
void CP_UI_Menu_DrawSatInfo(st_SatInfo src)
{
	if (!src.valid) return;

	sprintf(String, "%s", src.name);
	UI_PrintStringSmallest(String, 77, 0, false, true);
	sprintf(String, "%03d | %02d", src.currentAz / 10, src.currentEl / 10);
	UI_PrintStringSmallest(String, 77, 8, false, true);
	sprintf(String, "dV: %d", src.currentSpd);
	UI_PrintStringSmallest(String, 77, 16, false, true);
	sprintf(String, "%03d.%03d | %03d.%03d", 
		src.uplinkFreq / 100000, (src.uplinkFreq / 100) % 1000,
		src.downlinkFreq / 100000, (src.downlinkFreq / 100) % 1000);
	UI_PrintStringSmallest(String, 2, 49, false, true);
}
#endif