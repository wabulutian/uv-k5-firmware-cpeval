#ifndef __CP_DEF_H__
#define __CP_DEF_H__

#include <stdint.h>
#include <stdbool.h>
#include "cp_ref.h"

#define DEG2RAD (0.01745329251994329f)

const static uint32_t F_MIN = 2500000;
const static uint32_t F_MAX = 130000000;

// #define ENABLE_PASS	1

#define CP_I2C_ADDR_DEVICE				0x23
#define CP_I2C_ADDR_INFO				0	// basic info of coprocessor
#define CP_I2C_ADDR_TIME				1	// rtc time read/set
#define CP_I2C_ADDR_QTH_LL  			2	// QTH lat/lon
#define CP_I2C_ADDR_TLE_START			4	// tle upload
#define CP_I2C_ADDR_SAT_INFO_START  	14	
#define CP_I2C_ADDR_PASS_INFO_START 	24
#define CP_I2C_ADDR_FREQ_START  	    34	// freq

#define CP_I2C_SIZE_INFO				1
#define CP_I2C_SIZE_TIME				16
#define CP_I2C_SIZE_QTH_LL  			8
#define CP_I2C_SIZE_FREQ  			    8
#define CP_I2C_SIZE_TLE					213
#define CP_I2C_SIZE_SAT_INFO			89
#define CP_I2C_SIZE_PASS_INFO			362

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	int8_t zone;
	long long tv;
}st_Time;

typedef struct
{
	int32_t lon;
	int32_t lat;
}st_Site;
typedef union
{
	st_Site data;
	uint8_t array[8];
}un_Site;

typedef struct
{
	uint32_t up;
	uint32_t down;
}st_Freq;
typedef union
{
	st_Freq data;
	uint8_t array[8];
}un_Freq;

typedef struct
{
	char	l1[71];
	char	l2[71];
	char	l3[71];
}st_tle;
typedef union
{
	st_tle lines;
	char tle[71*3];
}un_tle;


typedef struct
{
	char 		name[71];
	uint8_t		valid;
	int16_t		currentAz;
	int16_t		currentEl;
	int16_t   	currentSpd;
	uint16_t   	rfu;
	uint32_t  	downlinkFreq;
	uint32_t  	uplinkFreq;
	uint8_t		chk;
}st_SatInfo;
typedef union
{
	st_SatInfo data;
	uint8_t array[89];
}un_SatInfo;

typedef struct
{
	int16_t  	azimuth;
	int16_t   elevation;
}st_AzEl10;

typedef struct
{
	uint8_t			active;
	uint8_t			aosMin;
	st_AzEl10  	azElList[90];
}st_PassInfo;
typedef union
{
	st_PassInfo data;
	uint8_t array[362];
}un_PassInfo;

typedef enum {
	CH_RX,
	CH_TX,
	CH_RXMASTER,
	CH_TXMASTER
}enum_FreqChannel;

typedef enum {
	NORMAL,
	FREQ_INPUT,
	MENU,
	MESSAGE
}enum_State;

typedef enum {
	FREQTRACK_OFF,
	FREQTRACK_FULLAUTO,
	FREQTRACK_SEMIAUTO
}enum_FrequencyTrackMode;

typedef enum {
	TRANSPONDER_NORMAL,
	TRANSPONDER_INVERSE
}enum_TransponderType;

typedef enum {
	TX_FM,
	TX_CW
}enum_TxMode;

typedef struct
{
	enum_FreqChannel		currentChannel;
	enum_FrequencyTrackMode	freqTrackMode;
	enum_TransponderType	transponderType;
	enum_TxMode				txMode;
	uint8_t					ctcssToneIdx;
}st_CPSettings;

typedef struct
{
	uint8_t 	chkbit;
	int8_t		bk4819TuneFreqOffset;
	uint8_t		CPStroageNum;
	uint8_t		rfu1;
	uint8_t		rfu2;
	uint8_t		rfu3;
	uint8_t		rfu4;
	uint8_t		rfu5;
}st_CPParams;
typedef union
{
	st_CPParams 	data;
	uint8_t				arr[8];
}un_CPParams;

typedef enum {
	MENU_NONE,
	MENU_FREQTRACK,
	MENU_TRANSPONDER,
	MENU_3RD
}enum_QuickMenuState;

#endif