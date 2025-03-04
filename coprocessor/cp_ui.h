#ifndef __CP_UI_H__
#define __CP_UI_H__

#include "external/printf/printf.h"
#include "cp_def.h"
#include "driver/st7565.h"
#include "driver/bk4819.h"
#include "ui/helper.h"
#include "radio.h"
#include "helper/measurements.h"


// LCD has poor resolution, 2 deg/step is enough
static int8_t sinLUT2deg100[180] = {0, 3, 7, 10, 14, 17, 21, 24, 28, 31, 34, 37, 41, 44, 47, 50, 53, 56, 59, 62, 64, 67, 69, 72, 74, 77, 79, 81, 83, 85, 87, 88, 90, 91, 93, 94, 95, 96, 97, 98, 98, 99, 99, 100, 100, 100, 100, 100, 99, 99, 98, 98, 97, 96, 95, 94, 93, 91, 90, 88, 87, 85, 83, 81, 79, 77, 74, 72, 69, 67, 64, 62, 59, 56, 53, 50, 47, 44, 41, 37, 34, 31, 28, 24, 21, 17, 14, 10, 7, 3, 0, -3, -7, -10, -14, -17, -21, -24, -28, -31, -34, -37, -41, -44, -47, -50, -53, -56, -59, -62, -64, -67, -69, -72, -74, -77, -79, -81, -83, -85, -87, -88, -90, -91, -93, -94, -95, -96, -97, -98, -98, -99, -99, -100, -100, -100, -100, -100, -99, -99, -98, -98, -97, -96, -95, -94, -93, -91, -90, -88, -87, -85, -83, -81, -79, -77, -74, -72, -69, -67, -64, -62, -59, -56, -53, -50, -47, -44, -41, -37, -34, -31, -28, -24, -21, -17, -14, -10, -7, -3};

static int8_t cosLUT2deg100[180] = 
{100, 100, 100, 99, 99, 98, 98, 97, 96, 95, 94, 93, 91, 90, 88, 87, 85, 83, 81, 79, 77, 74, 72, 69, 67, 64, 62, 59, 56, 53, 50, 47, 44, 41, 37, 34, 31, 28, 24, 21, 17, 14, 10, 7, 3, 0, -3, -7, -10, -14, -17, -21, -24, -28, -31, -34, -37, -41, -44, -47, -50, -53, -56, -59, -62, -64, -67, -69, -72, -74, -77, -79, -81, -83, -85, -87, -88, -90, -91, -93, -94, -95, -96, -97, -98, -98, -99, -99, -100, -100, -100, -100, -100, -99, -99, -98, -98, -97, -96, -95, -94, -93, -91, -90, -88, -87, -85, -83, -81, -79, -77, -74, -72, -69, -67, -64, -62, -59, -56, -53, -50, -47, -44, -41, -37, -34, -31, -28, -24, -21, -17, -14, -10, -7, -3, 0, 3, 7, 10, 14, 17, 21, 24, 28, 31, 34, 37, 41, 44, 47, 50, 53, 56, 59, 62, 64, 67, 69, 72, 74, 77, 79, 81, 83, 85, 87, 88, 90, 91, 93, 94, 95, 96, 97, 98, 98, 99, 99, 100, 100};

#define QUICKMENU_CELL_WIDTH		32
#define RADAR_CENTER_X				104
#define RADAR_CENTER_Y				40	
#define RSSI_METER_PADDING_LEFT		45

typedef enum
{
    ALL,
    SINGLE,
    SINGLE_WITH_PREDICT,
}enum_satRadarDisplayMode;

void UI_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool fill, bool bold);
void UI_DrawCircle(int centerx, int centery, int radius);
void CP_UI_DrawEmptyRadar(void);
void CP_UI_DrawFrequency(uint32_t rx10hz, uint32_t tx10hz, enum_FreqChannel channel, enum_FrequencyTrackMode mode, bool isFreqInputMode, char* p_freqInputString, bool isTx);
void CP_UI_DrawQuickMenu(char* item1, char* item2, char* item3, uint8_t sel);
void CP_UI_DrawRssi(int dbm, uint8_t s, double enable);
void CP_UI_DrawChannelIcon(enum_FreqChannel currentChannel);


void CP_UI_DrawStatusBarInMainScreen(st_time24MsgPack time, uint8_t battLevel, bool gnss, uint8_t freqStep, ModulationType modulation, BK4819_FilterBandwidth_t bw);
void CP_UI_DrawStatusBarInMenu(st_time24MsgPack time, uint8_t battLevel, bool gnss);
void CP_UI_Menu_DrawMenuList(char *title, uint8_t menuSelectIdx, char **itemList, uint8_t itemCount);
void CP_UI_Menu_DrawSatList(char *title, st_satStatusMsgPack *satInfoList, uint8_t selectedSatIdx, bool enableDoppler, uint8_t dopplerIdx);
void CP_UI_SubMenu_DrawSingleSatInfo(st_satStatusMsgPack *satInfoList, uint8_t selectedSatIdx);
void CP_UI_SubMenu_DrawGnssInfo_Detail(st_time24MsgPack *time, st_siteInfoMsgPack *site, char* gnrmc, int8_t tz, bool fixed);
void CP_UI_SubMenu_DrawGnssInfo(st_time24MsgPack *time, st_siteInfoMsgPack *site, bool fixed);
void CP_UI_SubMenu_DrawTleInfo(char* tleString, uint8_t slot, bool tleValid, bool isDeleteMode);

uint8_t CP_UI_DrawRadar(st_satStatusMsgPack *satList, uint8_t selectedIndex, st_satPredict120min_2min pred, enum_satRadarDisplayMode displayMode);

#endif