#ifndef __COPROCESSOR_H__
#define __COPROCESSOR_H__

#include "board.h"
#include "ARMCM0.h"

#include <stdbool.h>
#include <stdint.h>

#include "app/finput.h"
#include "radio.h"
#include "misc.h"
#include "settings.h"

#include "driver/system.h"
#include "driver/uart.h"
#include "driver/rtc.h"
#include "driver/gpio.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/keyboard.h"

#include "ui/helper.h"

#include "helper/battery.h"
#include "helper/measurements.h"

#include "bsp/dp32g030/uart.h"
#include "bsp/dp32g030/irq.h"
#include "bsp/dp32g030/syscon.h"
#include "bsp/dp32g030/dma.h"
#include "bsp/dp32g030/gpio.h"

#include "coprocessor/cp_def.h"
#include "coprocessor/cp_ref.h"
#include "coprocessor/cp_i2c.h"
#include "coprocessor/cp_uart.h"
#include "coprocessor/cp_ui.h"

typedef struct
{
	enum_FreqChannel		currentChannel;
	enum_TxMode				txMode;
	uint8_t					ctcssToneIdx;
    enum_TransponderType	transponderType;
	enum_FrequencyTrackMode	freqTrackMode;
}st_CpSettings;


void CP_APP_ExitCoprocessor(void);
void CP_APP_RunCoprocessor(void);
#endif