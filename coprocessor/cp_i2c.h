#ifndef __CP_DEF_H__
#define __CP_DEF_H__

#include "board.h"
#include "ARMCM0.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"
#include "driver/system.h"

#define CP_I2C_DIR_READ					0
#define CP_I2C_DIR_WRITE				1

#define CP_I2C_ADDR_DEVICE				0x20
#define CP_I2C_ADDR_INFO				0	// basic info of coprocessor
#define CP_I2C_ADDR_TIME				1	// rtc time read/set
#define CP_I2C_ADDR_QTH_GRID			2	// QTH grid read/set
#define CP_I2C_ADDR_QTH_LL_RO			3	// QTH lat/lon read
#define CP_I2C_ADDR_PRED_LEN			4	// predict length read/set
#define CP_I2C_ADDR_TLE					5	// tle upload
#define CP_I2C_ADDR_SAT_NUM_RO			6	// current storaged sat number
#define CP_I2C_ADDR_PASS_NUM_RO			7	// current storaged pass number
#define CP_I2C_ADDR_SAT_INFO_START_RO	8	// 20 SATs max
#define CP_I2C_ADDR_PASS_INFO_START_RO	28	// 40 PASSes max
#define CP_I2C_ADDR_SAT_CURRENT			69	// selected sat
#define CP_I2C_ADDR_SAT_STST_RO			70	// status of selected sat

#define CP_I2C_SIZE_INFO				1
#define CP_I2C_SIZE_TIME				16	// yymmddhhmmss+tv
#define CP_I2C_SIZE_QTH_GRID			6	// AA00AA
#define CP_I2C_SIZE_QTH_LL_RO			8	// int32, latlon x 1000000
#define CP_I2C_SIZE_PRED_LEN			1	// 24/48/72
#define CP_I2C_SIZE_TLE					284	// 3*71+freq
#define CP_I2C_SIZE_SAT_NUM_RO			1	// 20 max
#define CP_I2C_SIZE_PASS_NUM_RO			1	// 40 max
#define CP_I2C_SIZE_SAT_INFO			71	// name(71)+ul(4)+dl(4)
#define CP_I2C_SIZE_PASS_INFO			80	// satIdx(2)+aosTime(16)+losTime(16)+aosAz(2)+losAz(2)+maxElev(2)+passPlot((2+2)*10)
#define CP_I2C_SIZE_SAT_CURRENT			1	// 20 max
#define CP_I2C_SIZE_SAT_STAT_RO			6	// az(2)+el(2)+spd(2)

#endif