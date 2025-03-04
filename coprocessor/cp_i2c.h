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

void CP_I2C_Read(uint16_t Address, void *pBuffer, uint16_t len);
void CP_I2C_Write(uint16_t Address, const void *pBuffer, uint16_t len);

#endif