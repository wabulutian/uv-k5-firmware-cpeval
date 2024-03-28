#include "coprocessor/cp_ui.h"

uint8_t cp_menuItemNum = 5;
const char* cp_freqChangeStepDisp[] = {"100k", "10k ", "1k  ", "0.1k"};
static const char cp_menuList[][7] = {
	"PASS",
	"SAT",
	"QTH",
	"TIME",
	"PRED"
};
static char String[32];
static char String2[32];

void UI_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool fill)
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
			}
		}
        else if(y1>y2)
		{
			for(y2 = y2; y2 <= y1; y2 ++)
			{
				PutPixel(x1, y2, fill);
			}
		}
	}
	else if(DeltaY == 0)
	{
		for(x1 = x1; x1 <= x2; x1++)
		{
			PutPixel(x1, y1, fill);
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
			}
		}
		else if((k >= 1) || (k <= -1))
		{
			if(y1 <= y2)
			{
				for(y1 = y1; y1 <= y2; y1 ++)
				{
					PutPixel((int)((y1 - b) / k), y1, fill);
				}
			}
            else if(y1>y2)
			{
				for(y2 = y2; y2 <= y1; y2 ++)
				{
					PutPixel((int)((y2 - b) / k), y2, fill);
				}
			}
		}
	}
}


void UI_DrawCircle(int centerx, int centery, int radius) 
{
    // X-axis multiplied by 1.4 because the pixel shape is not square
    // So @radius represents the actual scale of the Y-axis
	for (int i = 1; i < 180; i ++) 
    {
		UI_DrawLine(centerx + radius * 1.4f * cosLUT2deg[i], centery + radius * sinLUT2deg[i], 
				centerx + radius * 1.4f * cosLUT2deg[i - 1], centery + radius * sinLUT2deg[i - 1],
				true);
	}
}

void CP_UI_DrawEmptyRadar(void)
{
    UI_DrawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, 15);
	UI_DrawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, 7);
	UI_DrawLine(RADAR_CENTER_X - 21, RADAR_CENTER_Y, RADAR_CENTER_X + 21, RADAR_CENTER_Y, true);
	UI_DrawLine(RADAR_CENTER_X, RADAR_CENTER_Y - 15, RADAR_CENTER_X, RADAR_CENTER_Y + 15, true);
}

void CP_UI_DrawRadar(st_PassInfo pass) 
{
    uint16_t az;
    uint8_t el;
    uint16_t satx, saty;
    int16_t i;

	CP_UI_DrawEmptyRadar();

    // Track
    for (i = 1; i < 10; i ++)
    {
        uint16_t az0 = pass.azimuthList[i-1];
        uint8_t el0 = pass.elevationList[i-1];
        uint16_t satx0 = RADAR_CENTER_X + (el0 / 6) * cosLUT2deg[az0 / 2] * 1.4f;
	    uint16_t saty0 = RADAR_CENTER_Y + (el0 / 6) * sinLUT2deg[az0 / 2];

        uint16_t az1 = pass.azimuthList[i];
        uint8_t el1 = pass.elevationList[i];
        uint16_t satx1 = RADAR_CENTER_X + (el1 / 6) * cosLUT2deg[az1 / 2] * 1.4f;
	    uint16_t saty1 = RADAR_CENTER_Y + (el1 / 6) * sinLUT2deg[az1 / 2];

        UI_DrawLine(satx0, saty0, satx1, saty1, true);
    }

    // Current position
    az = pass.sat->currentAz;
    el = pass.sat->currentEl;
	satx = RADAR_CENTER_X + (el / 6) * cosLUT2deg[az / 2] * 1.4f;
	saty = RADAR_CENTER_Y + (el / 6) * sinLUT2deg[az / 2];
	for (i = -2; i < 3; i ++) 
    {
		PutPixel(satx + i, saty + i, true);
		PutPixel(satx + i, saty - i, true);
		PutPixel(satx, saty + i, true);
		PutPixel(satx + i, saty, true);
	}
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

	sprintf(String, "Rx");
	UI_PrintStringSmallest(String, 5, 1, false, true);
	if (!isTx) 
	{
		sprintf(String, "Tx");
		UI_PrintStringSmallest(String, 5, 9, false, true);
	} else {
		for (int i = 0; i < 15; i ++) {
			gFrameBuffer[1][i] = 0b11111111;
		}
		sprintf(String, "Tx");
		UI_PrintStringSmallest(String, 5, 9, false, false);
	}
}

void CP_UI_DrawQuickMenu(char* item1, char* item2, char* item3, uint8_t sel, bool enable)
{
    for (int i = 0; i < 3; i ++) 
    {
		if (sel == i+1) 
        {
			for (int j = 0; j < QUICKMENU_CELL_WIDTH; j ++) 
            {
				gFrameBuffer[3 + i][j] = 0b11111110;
			}
		}
    }
    UI_PrintStringSmallest(item1, 2, 3 * 8 + 2, false, !(enable && sel == 1));
    UI_PrintStringSmallest(item2, 2, 4 * 8 + 2, false, !(enable && sel == 2));
    UI_PrintStringSmallest(item3, 2, 5 * 8 + 2, false, !(enable && sel == 3));
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

void CP_UI_DrawDateTime(st_Time time)
{
    //sprintf(String, "%d.%02d.%02d %02d:%02d:%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
	sprintf(String, "2024.03.27 23:01:00");
	UI_PrintStringSmallest(String, 2, 50, false, true);
}

void CP_UI_DrawStatusBar(uint8_t freqStep, ModulationType modulation, BK4819_FilterBandwidth_t bw, uint8_t battLevel)
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

	uint8_t stepIndcStart = freqStep == 3 ? 76 : (48 + freqStep * 7);
	for (int i = 0; i < 6; i ++) 
	{
		gStatusLine[i + stepIndcStart] |= 0b01000000;
		gFrameBuffer[2][i + stepIndcStart] |= 0b00000001;
	}

	sprintf(String, "%s %s",
			modulationTypeOptions[modulation],
			bwNames[bw]);
	UI_PrintStringSmallest(String, 75, 0, true, true);
}

void CP_UI_Menu_DrawMenuList(uint8_t menuSelectIdx)
{
	uint8_t offset = Clamp(menuSelectIdx - 2, 0, cp_menuItemNum - 5);
	for (int i = 0; i < 5; ++i) {
		const char *s = cp_menuList[i + offset];
		bool isCurrent = menuSelectIdx == i + offset;
		if (isCurrent) {
			UI_PrintStringSmallBold(s, 0, 48, i);
		} else {
			UI_PrintStringSmall(s, 0, 48, i);
		}
	}

	for (int i = 0; i < 7; i++) {
		gFrameBuffer[i][49] = 0xFF;
	}
}