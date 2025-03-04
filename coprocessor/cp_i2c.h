#ifndef __CP_I2C_H__
#define __CP_I2C_H__

#include "board.h"
#include "ARMCM0.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"
#include "driver/system.h"
#include "coprocessor/cp_def.h"

#define CP_I2C_DIR_READ					(0U)
#define CP_I2C_DIR_WRITE				(1U)

void CP_I2C_Info(void *pBuffer);
void CP_I2C_RTC(void *pBuffer, uint8_t rw);
void CP_I2C_SATINFO(void *pBuffer, uint8_t index);
void CP_I2C_TLE(void *pBuffer, uint8_t index);
void CP_I2C_Site(void *pBuffer);
void CP_I2C_Freq(void *pBuffer, uint8_t index);
void CP_I2C_PASSINFO(void *pBuffer, uint8_t index);

#endif