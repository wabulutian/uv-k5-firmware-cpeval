/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "menu.h"
#include "../app/dtmf.h"
#include "../bitmaps.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "../frequencies.h"
#include "../helper/battery.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../settings.h"
#include "helper.h"
#include "inputbox.h"
#include "ui.h"
#include <string.h>

static const char MenuList[][7] = {
    // 0x00
    "SQL",
    "STEP",
    "TXP",
    "R_DCS",
    "R_CTCS",
    "T_DCS",
    "T_CTCS",
    "SFT-D",
    // 0x08
    "OFFSET",
    "W/N",
    "SCR",
    "BCL",
    "MEM-CH",
    "SAVE",
    "VOX",
    "BACKLI",
    // 0x10
    "DW",
    "WX",
    "BEEP",
    "TOT",
    "VOICE",
    "SC-REV",
    "MDF",
    "AUTOLK",
    // 0x18
    "S-ADD1",
    "S-ADD2",
    "STE",
    "RP-STE",
    "MIC",
    "1-CALL",
    "S-LIST",
    "SLIST1",
    // 0x20
    "SLIST2",
    "ANI-ID",
    "UPCODE",
    "DWCODE",
    "D-ST",
    "D-RSP",
    "D-HOLD",
    // 0x28
    "D-PRE",
    "PTT-ID",
    "D-DCD",
    "D-LIST",
    "PONMSG",
    "ROGER",
    "VOL",
    "MODUL",
    // 0x30
    "DEL-CH",
    "RESET",
    "350TX",
    "F-LOCK",
    "200TX",
    "500TX",
    "ALL_TX",
    // 0x38
    "SCREN",
};

static const char gSubMenu_TXP[3][5] = {
    "LOW",
    "MID",
    "HIGH",
};

static const char gSubMenu_SFT_D[3][4] = {
    "OFF",
    "+",
    "-",
};

static const char gSubMenu_W_N[2][7] = {
    "WIDE",
    "NARROW",
};

static const char gSubMenu_SAVE[5][4] = {
    "OFF", "1:1", "1:2", "1:3", "1:4",
};

static const char gSubMenu_CHAN[3][7] = {
    "OFF",
    "CHAN_A",
    "CHAN_B",
};

static const char gSubMenu_VOICE[3][4] = {
    "OFF",
    "CHI",
    "ENG",
};

static const char gSubMenu_SC_REV[3][3] = {
    "TO",
    "CO",
    "SE",
};

static const char gSubMenu_MDF[4][7] = {"FREQ", "CHAN", "NAME", "NAME+F"};

static const char gSubMenu_D_RSP[4][6] = {
    "NULL",
    "RING",
    "REPLY",
    "BOTH",
};

static const char gSubMenu_PTT_ID[4][5] = {
    "OFF",
    "BOT",
    "EOT",
    "BOTH",
};

static const char gSubMenu_PONMSG[3][5] = {
    "FULL",
    "MSG",
    "VOL",
};

static const char gSubMenu_ROGER[3][6] = {
    "OFF",
    "ROGER",
    "MDC",
};

static const char gSubMenu_RESET[2][4] = {
    "VFO",
    "ALL",
};

static const char gSubMenu_F_LOCK[5][8] = {
    "OFF", "FCC", "CE", "GB", "LPD PMR",
};
const char gSubMenuBacklight[8][7] = {"OFF",   "5 sec", "10 sec", "20 sec",
                                      "1 min", "2 min", "ON"};

static const char *defaultEnableDisable[3] = {"DEFAULT", "ENABLE", "DISABLE"};
static const char *offOn[3] = {"OFF", "ON"};

bool gIsInSubMenu;

uint8_t gMenuCursor;
int8_t gMenuScrollDirection;
uint32_t gSubMenuSelection;

void UI_DisplayMenu(void) {
  char String[16];
  char Contact[16];
  uint8_t i;

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  uint8_t offset = Clamp(gMenuCursor - 2, 0, gMenuListCount - 5);
  for (int i = 0; i < 5; ++i) {
    const char *s = MenuList[i + offset];
    bool isCurrent = gMenuCursor == i + offset;
    if (isCurrent) {
      UI_PrintStringSmallBold(s, 0, 48, i);
    } else {
      UI_PrintStringSmall(s, 0, 48, i);
    }
  }

  uint8_t menuScrollbarPosY =
      ConvertDomain(gMenuCursor, 0, gMenuListCount, 0, 7 * 8 - 3);

  for (i = 0; i < 7; i++) {
    gFrameBuffer[i][49] = 0xFF;
  }

  for (uint8_t a = 0; a < 3; a++) {
    PutPixel(48, menuScrollbarPosY + a, true);
    PutPixel(50, menuScrollbarPosY + a, true);
  }

  sprintf(String, "%03u", gMenuCursor + 1);
  UI_PrintStringSmall(String, 32, 48, 6);
  if (gIsInSubMenu) {
    memcpy(gFrameBuffer[0] + 50, BITMAP_CurrentIndicator,
           sizeof(BITMAP_CurrentIndicator));
  }

  memset(String, 0, sizeof(String));

  switch (gMenuCursor) {
  case MENU_SQL:
  case MENU_MIC:
    sprintf(String, "%d", gSubMenuSelection);
    break;

  case MENU_STEP:
    sprintf(String, "%d.%02dKHz", StepFrequencyTable[gSubMenuSelection] / 100,
            StepFrequencyTable[gSubMenuSelection] % 100);
    break;

  case MENU_TXP:
    strcpy(String, gSubMenu_TXP[gSubMenuSelection]);
    break;

  case MENU_R_DCS:
  case MENU_T_DCS:
    if (gSubMenuSelection == 0) {
      strcpy(String, "OFF");
    } else if (gSubMenuSelection < 105) {
      sprintf(String, "D%03oN", DCS_Options[gSubMenuSelection - 1]);
    } else {
      sprintf(String, "D%03oI", DCS_Options[gSubMenuSelection - 105]);
    }
    break;

  case MENU_R_CTCS:
  case MENU_T_CTCS:
    if (gSubMenuSelection == 0) {
      strcpy(String, "OFF");
    } else {
      sprintf(String, "%d.%dHz", CTCSS_Options[gSubMenuSelection - 1] / 10,
              CTCSS_Options[gSubMenuSelection - 1] % 10);
    }
    break;

  case MENU_SFT_D:
    strcpy(String, gSubMenu_SFT_D[gSubMenuSelection]);
    break;

  case MENU_OFFSET:
    if (!gIsInSubMenu || gInputBoxIndex == 0) {
      sprintf(String, "%d.%05d", gSubMenuSelection / 100000,
              gSubMenuSelection % 100000);
      break;
    }
    for (i = 0; i < 3; i++) {
      if (gInputBox[i] == 10) {
        String[i] = '-';
      } else {
        String[i] = gInputBox[i] + '0';
      }
    }
    String[3] = '.';
    for (i = 3; i < 6; i++) {
      if (gInputBox[i] == 10) {
        String[i + 1] = '-';
      } else {
        String[i + 1] = gInputBox[i] + '0';
      }
    }
    String[7] = '-';
    String[8] = '-';
    String[9] = 0;
    String[10] = 0;
    String[11] = 0;
    break;

  case MENU_W_N:
    strcpy(String, gSubMenu_W_N[gSubMenuSelection]);
    break;

  case MENU_SCR:
  case MENU_VOX:
    if (gSubMenuSelection == 0) {
      strcpy(String, "OFF");
    } else {
      sprintf(String, "%d", gSubMenuSelection);
    }
    break;

  case MENU_ABR:
    strcpy(String, gSubMenuBacklight[gSubMenuSelection]);
    break;

  case MENU_ALL_TX:
    strcpy(String, defaultEnableDisable[gSubMenuSelection]);
    break;

  case MENU_BCL:
  case MENU_BEEP:
  case MENU_AUTOLK:
  case MENU_S_ADD1:
  case MENU_S_ADD2:
  case MENU_D_ST:
  case MENU_D_DCD:
  case MENU_350TX:
  case MENU_200TX:
  case MENU_500TX:
  case MENU_SCREN:
  case MENU_STE:
  case MENU_AM:
    strcpy(String, offOn[gSubMenuSelection]);
    break;

  case MENU_MEM_CH:
  case MENU_1_CALL:
  case MENU_DEL_CH:
    UI_GenerateChannelStringEx(
        String, RADIO_CheckValidChannel((uint16_t)gSubMenuSelection, false, 0),
        (uint8_t)gSubMenuSelection);
    break;

  case MENU_SAVE:
    strcpy(String, gSubMenu_SAVE[gSubMenuSelection]);
    break;

  case MENU_TDR:
  case MENU_WX:
    strcpy(String, gSubMenu_CHAN[gSubMenuSelection]);
    break;

  case MENU_TOT:
    if (gSubMenuSelection == 0) {
      strcpy(String, "OFF");
    } else {
      sprintf(String, "%dmin", gSubMenuSelection);
    }
    break;

  case MENU_VOICE:
    strcpy(String, gSubMenu_VOICE[gSubMenuSelection]);
    break;

  case MENU_SC_REV:
    strcpy(String, gSubMenu_SC_REV[gSubMenuSelection]);
    break;

  case MENU_MDF:
    strcpy(String, gSubMenu_MDF[gSubMenuSelection]);
    break;

  case MENU_RP_STE:
    if (gSubMenuSelection == 0) {
      strcpy(String, "OFF");
    } else {
      sprintf(String, "%d*100ms", gSubMenuSelection);
    }
    break;

  case MENU_S_LIST:
    sprintf(String, "LIST%d", gSubMenuSelection);
    break;

  case MENU_ANI_ID:
    strcpy(String, gEeprom.ANI_DTMF_ID);
    break;

  case MENU_UPCODE:
    strcpy(String, gEeprom.DTMF_UP_CODE);
    break;

  case MENU_DWCODE:
    strcpy(String, gEeprom.DTMF_DOWN_CODE);
    break;

  case MENU_D_RSP:
    strcpy(String, gSubMenu_D_RSP[gSubMenuSelection]);
    break;

  case MENU_D_HOLD:
    sprintf(String, "%ds", gSubMenuSelection);
    break;

  case MENU_D_PRE:
    sprintf(String, "%d*10ms", gSubMenuSelection);
    break;

  case MENU_PTT_ID:
    strcpy(String, gSubMenu_PTT_ID[gSubMenuSelection]);
    break;

  case MENU_D_LIST:
    gIsDtmfContactValid =
        DTMF_GetContact((uint8_t)gSubMenuSelection - 1, Contact);
    if (!gIsDtmfContactValid) {
      // Ghidra being weird again...
      memcpy(String, "NULL\0\0\0", 8);
    } else {
      memcpy(String, Contact, 8);
    }
    break;

  case MENU_PONMSG:
    strcpy(String, gSubMenu_PONMSG[gSubMenuSelection]);
    break;

  case MENU_ROGER:
    strcpy(String, gSubMenu_ROGER[gSubMenuSelection]);
    break;

  case MENU_VOL:
    sprintf(String, "%d.%02dV", gBatteryVoltageAverage / 100,
            gBatteryVoltageAverage % 100);
    break;

  case MENU_RESET:
    strcpy(String, gSubMenu_RESET[gSubMenuSelection]);
    break;

  case MENU_F_LOCK:
    strcpy(String, gSubMenu_F_LOCK[gSubMenuSelection]);
    break;
  }

  UI_PrintString(String, 50, 127, 2, 8, true);

  if (gMenuCursor == MENU_OFFSET) {
    UI_PrintString("MHz", 50, 127, 4, 8, true);
  }

  if ((gMenuCursor == MENU_RESET || gMenuCursor == MENU_MEM_CH ||
       gMenuCursor == MENU_DEL_CH) &&
      gAskForConfirmation) {
    if (gAskForConfirmation == 1) {
      strcpy(String, "SURE?");
    } else {
      strcpy(String, "WAIT!");
    }
    UI_PrintString(String, 50, 127, 4, 8, true);
  }

  if ((gMenuCursor == MENU_R_CTCS || gMenuCursor == MENU_R_DCS) &&
      gCssScanMode != CSS_SCAN_MODE_OFF) {
    UI_PrintString("SCAN", 50, 127, 4, 8, true);
  }

  if (gMenuCursor == MENU_UPCODE) {
    if (strlen(gEeprom.DTMF_UP_CODE) > 8) {
      UI_PrintString(gEeprom.DTMF_UP_CODE + 8, 50, 127, 4, 8, true);
    }
  }
  if (gMenuCursor == MENU_DWCODE) {
    if (strlen(gEeprom.DTMF_DOWN_CODE) > 8) {
      UI_PrintString(gEeprom.DTMF_DOWN_CODE + 8, 50, 127, 4, 8, true);
    }
  }
  if (gMenuCursor == MENU_D_LIST && gIsDtmfContactValid) {
    Contact[11] = 0;
    memcpy(&gDTMF_ID, Contact + 8, 4);
    sprintf(String, "ID:%s", Contact + 8);
    UI_PrintString(String, 50, 127, 4, 8, true);
  }

  if (gMenuCursor == MENU_R_CTCS || gMenuCursor == MENU_T_CTCS ||
      gMenuCursor == MENU_R_DCS || gMenuCursor == MENU_T_DCS ||
      gMenuCursor == MENU_D_LIST) {
    uint8_t Offset;

    NUMBER_ToDigits((uint8_t)gSubMenuSelection, String);
    Offset = (gMenuCursor == MENU_D_LIST) ? 2 : 3;
    UI_DisplaySmallDigits(Offset, String + (8 - Offset), 105, 0);
  }

  if (gMenuCursor == MENU_SLIST1 || gMenuCursor == MENU_SLIST2) {
    i = gMenuCursor - MENU_SLIST1;

    if (gSubMenuSelection == 0xFF) {
      sprintf(String, "NULL");
    } else {
      UI_GenerateChannelStringEx(String, true, (uint8_t)gSubMenuSelection);
    }

    if (gSubMenuSelection == 0xFF || !gEeprom.SCAN_LIST_ENABLED[i]) {
      UI_PrintString(String, 50, 127, 2, 8, true);
    } else {
      UI_PrintString(String, 50, 127, 0, 8, true);
      if (IS_MR_CHANNEL(gEeprom.SCANLIST_PRIORITY_CH1[i])) {
        sprintf(String, "PRI1:%d", gEeprom.SCANLIST_PRIORITY_CH1[i] + 1);
        UI_PrintString(String, 50, 127, 2, 8, true);
      }
      if (IS_MR_CHANNEL(gEeprom.SCANLIST_PRIORITY_CH2[i])) {
        sprintf(String, "PRI2:%d", gEeprom.SCANLIST_PRIORITY_CH2[i] + 1);
        UI_PrintString(String, 50, 127, 4, 8, true);
      }
    }
  }

  ST7565_BlitFullScreen();
}
