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

#if defined(ENABLE_FMRADIO)
#include "app/fm.h"
#endif
#include "app/scanner.h"
#include "audio.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"

#define DECREMENT_AND_TRIGGER(cnt, flag)                                       \
  do {                                                                         \
    if (cnt) {                                                                 \
      if (--cnt == 0) {                                                        \
        flag = true;                                                           \
      }                                                                        \
    }                                                                          \
  } while (0)

static volatile uint32_t gGlobalSysTickCounter;

void SystickHandler(void);

void SystickHandler(void) {
  gGlobalSysTickCounter++;
  gNextTimeslice = true;
  if ((gGlobalSysTickCounter % 50) == 0) {
    gNextTimeslice500ms = true;
    DECREMENT_AND_TRIGGER(gTxTimerCountdown, gTxTimeoutReached);
  }
  if ((gGlobalSysTickCounter & 3) == 0) {
    gNextTimeslice40ms = true;
  }
  if (gSystickCountdown2) {
    gSystickCountdown2--;
  }
  if (gFoundCDCSSCountdown) {
    gFoundCDCSSCountdown--;
  }
  if (gFoundCTCSSCountdown) {
    gFoundCTCSSCountdown--;
  }
  if (gCurrentFunction == FUNCTION_FOREGROUND) {
    DECREMENT_AND_TRIGGER(gBatterySaveCountdown, gSchedulePowerSave);
  }
  if (gCurrentFunction == FUNCTION_POWER_SAVE) {
    DECREMENT_AND_TRIGGER(gBatterySave, gBatterySaveCountdownExpired);
  }

  if (gScanState == SCAN_OFF && gCssScanMode == CSS_SCAN_MODE_OFF &&
      gEeprom.DUAL_WATCH != DUAL_WATCH_OFF) {
    if (gCurrentFunction != FUNCTION_MONITOR &&
        gCurrentFunction != FUNCTION_TRANSMIT) {
      if (gCurrentFunction != FUNCTION_RECEIVE) {
        DECREMENT_AND_TRIGGER(gDualWatchCountdown, gScheduleDualWatch);
      }
    }
  }

  if (gScanState != SCAN_OFF || gCssScanMode == CSS_SCAN_MODE_SCANNING) {
    if (gCurrentFunction != FUNCTION_MONITOR &&
        gCurrentFunction != FUNCTION_TRANSMIT) {
      DECREMENT_AND_TRIGGER(ScanPauseDelayIn10msec, gScheduleScanListen);
    }
  }

  DECREMENT_AND_TRIGGER(gTailNoteEliminationCountdown, gFlagTteComplete);

#if defined(ENABLE_FMRADIO)
  if (gFM_ScanState != FM_SCAN_OFF && gCurrentFunction != FUNCTION_MONITOR) {
    if (gCurrentFunction != FUNCTION_TRANSMIT &&
        gCurrentFunction != FUNCTION_RECEIVE) {
      DECREMENT_AND_TRIGGER(gFmPlayCountdown, gScheduleFM);
    }
  }
#endif
  if (gVoxStopCountdown) {
    gVoxStopCountdown--;
  }
}
