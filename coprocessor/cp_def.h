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

#define CP_MENU_ITEM_NUMBER				3

typedef struct
{
	uint8_t	hh;		//2
	uint8_t mm;		//2
	uint8_t ss;		//2
	int8_t tz;		//2
}st_time24MsgPack;

typedef struct
{
	uint16_t siteLat_Deg;
	uint16_t siteLon_Deg;
	uint16_t siteLat_0_001Deg;
	uint16_t siteLon_0_001Deg;
	char NS;
	char EW;
	char siteMaidenhead[8];
}st_siteInfoMsgPack;

typedef struct
{
	char name[11];						//11
	uint8_t valid;						//1
	uint32_t  	downlinkFreq;	//4
	uint32_t  	uplinkFreq;		//4
	int16_t satAz_1Deg;			//2
	int16_t satEl_1Deg;			//2
	int16_t satSpd_1mps;			//2
	int16_t downlinkDoppler;	//2
	int16_t	uplinkDoppler;		//2
}st_satStatusMsgPack;	//30

typedef struct
{
	uint8_t az_2Deg[60];
	int8_t	el_2Deg[60];
	uint16_t nextEvent_sec;
	uint8_t nextEvent_mm;
	uint8_t nextEvent_ss;
}st_satPredict120min_2min;//124

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
	SUBMENU,
	MESSAGE
}enum_State;

typedef enum {
	GNSS,
	SAT_INFO,
	IMPORT_TLE
}enum_SubMenu;

typedef enum {
	FREQTRACK_OFF,
	FREQTRACK_FM,
	FREQTRACK_LINEAR
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
	enum_TxMode				txMode;
	uint8_t					ctcssToneIdx;
    enum_TransponderType	transponderType;
	enum_FrequencyTrackMode	freqTrackMode;
}st_CpSettings;

typedef enum {
	MENU_NONE,
	MENU_FREQTRACK,
	MENU_TRANSPONDER,
	MENU_3RD
}enum_QuickMenuState;

#endif