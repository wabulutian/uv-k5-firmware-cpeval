#ifndef __SATPASS_DEF_H__
#define __SATPASS_DEF_H__

#include <stdint.h>
#include <stdbool.h>
#include "satpass_ref.h"

#define DEG2RAD (3.141592653589793f / 180.0f)

#define SATPASS_STROAGE_SIZE			5
#define	SATPASS_FREQ_NUM				184
#define SATPASS_FREQ_UPDATE_INTERVAL	5

#define SATPASS_EEPROM_BASE         	0x2100
#define SATPASS_EEPROM_SIZE   			  0x780
#define SATPASS_UNION_SIZE				(sizeof(st_PassInfo))

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

typedef union {
	uint8_t raw[16];
	char string[16];
}un_SatName;

typedef struct
{
	un_SatName 	name;        	//16
	st_Time     aosTv;       	//12
	uint32_t  	downlinkFreq;	//4
	uint32_t  	uplinkFreq;		//4
	uint16_t  	totalFreqNum;	//2
	int16_t   	speedList[SATPASS_FREQ_NUM];  		//2*point	//1800s = 30min
	uint16_t  	satAzimuthList[SATPASS_FREQ_NUM];		//2*point
	int8_t    	satElevationList[SATPASS_FREQ_NUM];	//1*point
	int8_t    	isInverseTransponder;					//1
	uint8_t		rfu;	//1
	uint16_t	rfu2;	//1
}st_PassInfo;	//1888 Byte
typedef union 
{
  st_PassInfo  data;
  uint8_t     arr[SATPASS_UNION_SIZE];
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
	DATA_RECV,
	SAT_MENU,
	TIME_INPUT
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
}st_SatpassSettings;

typedef struct
{
	uint8_t 	chkbit;
	int8_t		bk4819TuneFreqOffset;
	uint8_t		satpassStroageNum;
	uint8_t		rfu1;
	uint8_t		rfu2;
	uint8_t		rfu3;
	uint8_t		rfu4;
	uint8_t		rfu5;
}st_SatpassParams;
typedef union
{
	st_SatpassParams 	data;
	uint8_t				arr[8];
}un_SatpassParams;

typedef enum {
	MENU_NONE,
	MENU_TRANSPONDER,
	MENU_FREQTRACK,
	MENU_3RD,
}enum_QuickMenuState;



#endif