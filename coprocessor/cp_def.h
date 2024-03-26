#ifndef __CP_DEF_H__
#define __CP_DEF_H__

#include <stdint.h>
#include <stdbool.h>
#include "cp_ref.h"

#define DEG2RAD (0.01745329251994329f)

const static uint32_t F_MIN = 2500000;
const static uint32_t F_MAX = 130000000;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t rfu;
	long long tv;
}st_Time;


typedef struct
{
	char 		name[71];
	int16_t		currentAz;
	int16_t		currentEl;
	int16_t   	currentSpd;
	uint32_t  	downlinkFreq;
	uint32_t  	uplinkFreq;
}st_SatInfo;


typedef struct
{
	st_SatInfo*	sat;
	st_Time     aosTv;
	st_Time     losTv;
	int16_t		aosAz;
	int16_t		losAz;
	int16_t		maxElev;
	int16_t  	azimuthList[10];
	int16_t    	elevationList[10];
}st_PassInfo;

typedef enum {
	CH_RX,
	CH_TX,
	CH_RXMASTER,
	CH_TXMASTER
}enum_FreqChannel;

typedef enum {
	NORMAL,
	FREQ_INPUT,
	MENU
}enum_State;

typedef enum {
	TRANSPONDER_NORMAL,
	TRANSPONDER_INVERSE
}enum_TransponderType;

typedef enum {
	FREQTRACK_FULLAUTO,
	FREQTRACK_SEMIAUTO
}enum_FrequencyTrackMode;

typedef enum {
	TX_FM,
	TX_CW
}enum_TxMode;

typedef struct
{
	enum_FreqChannel		currentChannel;
	enum_TransponderType	transponderType;
	enum_FrequencyTrackMode	freqTrackMode;
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
	MENU_TRANSPONDER,
	MENU_FREQTRACK,
	MENU_3RD,
}enum_QuickMenuState;

#endif