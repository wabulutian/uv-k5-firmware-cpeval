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

#ifndef RADIO_H
#define RADIO_H

#include "dcs.h"
#include <stdbool.h>
#include <stdint.h>

enum {
  MR_CH_SCANLIST1 = (1U << 7),
  MR_CH_SCANLIST2 = (1U << 6),
  MR_CH_BAND_MASK = 0x0FU,
};

enum {
  RADIO_CHANNEL_UP = 0x01U,
  RADIO_CHANNEL_DOWN = 0xFFU,
};

enum {
  BANDWIDTH_WIDE = 0U,
  BANDWIDTH_NARROW = 1U,
};

enum PTT_ID_t {
  PTT_ID_OFF = 0U,
  PTT_ID_BOT = 1U,
  PTT_ID_EOT = 2U,
  PTT_ID_BOTH = 3U,
};

typedef enum PTT_ID_t PTT_ID_t;

enum STEP_Setting_t {
  STEP_0_01kHz,
  STEP_0_1kHz,
  STEP_0_5kHz,
  STEP_1_0kHz,

  STEP_2_5kHz,
  STEP_5_0kHz,
  STEP_6_25kHz,
  STEP_8_33kHz,
  STEP_10_0kHz,
  STEP_12_5kHz,
  STEP_25_0kHz,
  STEP_100_0kHz,
};

extern const char *modulationTypeOptions[];
extern const char *vfoStateNames[];
extern const char *powerNames[];
extern const char *bwNames[];
extern const char *deviationNames[];

typedef enum ModulationType {
  MOD_FM,
  MOD_AM,
  MOD_USB,
  MOD_BYP,
  MOD_RAW,
} ModulationType;

typedef enum STEP_Setting_t STEP_Setting_t;

enum VfoState_t {
  VFO_STATE_NORMAL = 0U,
  VFO_STATE_BUSY = 1U,
  VFO_STATE_BAT_LOW = 2U,
  VFO_STATE_TX_DISABLE = 3U,
  VFO_STATE_TIMEOUT = 4U,
  VFO_STATE_ALARM = 5U,
  VFO_STATE_VOL_HIGH = 6U,
};

typedef enum VfoState_t VfoState_t;

typedef struct {
  uint32_t Frequency;
  DCS_CodeType_t CodeType;
  uint8_t Code;
  uint8_t Padding[2];
} FREQ_Config_t;

typedef struct VFO_Info_t {
  FREQ_Config_t ConfigRX;
  FREQ_Config_t ConfigTX;
  FREQ_Config_t *pRX;
  FREQ_Config_t *pTX;
  uint32_t FREQUENCY_OF_DEVIATION;
  uint16_t StepFrequency;
  uint8_t CHANNEL_SAVE;
  uint8_t FREQUENCY_DEVIATION_SETTING;
  uint8_t SquelchOpenRSSIThresh;
  uint8_t SquelchOpenNoiseThresh;
  uint8_t SquelchCloseGlitchThresh;
  uint8_t SquelchCloseRSSIThresh;
  uint8_t SquelchCloseNoiseThresh;
  uint8_t SquelchOpenGlitchThresh;
  STEP_Setting_t STEP_SETTING;
  uint8_t OUTPUT_POWER;
  uint8_t TXP_CalculatedSetting;
  bool FrequencyReverse;
  uint8_t SCRAMBLING_TYPE;
  uint8_t CHANNEL_BANDWIDTH;
  uint8_t SCANLIST1_PARTICIPATION;
  uint8_t SCANLIST2_PARTICIPATION;
  uint8_t Band;
  uint8_t DTMF_DECODING_ENABLE;
  PTT_ID_t DTMF_PTT_ID_TX_MODE;
  uint8_t BUSY_CHANNEL_LOCK;
  uint8_t AM_CHANNEL_MODE;
  ModulationType ModulationType;
  char Name[16];
} VFO_Info_t;

extern VFO_Info_t *gTxVfo;
extern VFO_Info_t *gRxVfo;
extern VFO_Info_t *gCurrentVfo;

extern DCS_CodeType_t gCurrentCodeType;
extern DCS_CodeType_t gSelectedCodeType;
extern uint8_t gSelectedCode;

extern STEP_Setting_t gStepSetting;

extern VfoState_t VfoState[2];

bool RADIO_CheckValidChannel(uint16_t ChNum, bool bCheckScanList,
                             uint8_t RadioNum);
uint8_t RADIO_FindNextChannel(uint8_t ChNum, int8_t Direction,
                              bool bCheckScanList, uint8_t RadioNum);
void RADIO_InitInfo(VFO_Info_t *pInfo, uint8_t ChannelSave, uint8_t ChIndex,
                    uint32_t Frequency);
void RADIO_ConfigureChannel(uint8_t RadioNum, uint32_t Arg);
void RADIO_ConfigureSquelchAndOutputPower(VFO_Info_t *pInfo);
uint32_t GetOffsetedF(VFO_Info_t *pInfo, uint32_t f);
void RADIO_ApplyOffset(VFO_Info_t *pInfo);
void RADIO_SelectVfos(void);
void RADIO_SetupRegisters(bool bSwitchToFunction0);
void RADIO_SetTxParameters(void);

void RADIO_SetVfoState(VfoState_t State);
void RADIO_PrepareTX(void);
void RADIO_EnableCxCSS(void);
void RADIO_PrepareCssTX(void);
void RADIO_SendEndOfTransmission(void);

#endif
