#ifndef __SIMPLE_PROTOCOL_H__
#define __SIMPLE_PROTOCOL_H__

#include "board.h"
#include "driver/uart.h"
#include "bsp/dp32g030/uart.h"
#include "bsp/dp32g030/dma.h"
#include "bsp/dp32g030/gpio.h"
#include "ARMCM0.h"
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define UART_TRANSMIT(x, y)	UART_Send(x, y)
#define RXRB_INDEX(x, y) (((x) + (y)) % sizeof(SProto_rxBuffer))
#define	DATA_PKT_DATA_NUM	128

#define	SPROTO_MSG_HEADER_SOM	0xAB
#define	SPROTO_MSG_PKT_SOP		0xAA
#define	SPROTO_MSG_PKT_EOP		0xEE

#define SPROTO_MSG_TYPE_RPRT	0x0
#define	SPROTO_MSG_TYPE_DATA	0x1

#define SPROTO_RB_SIZE       256

#define MSG_OK							(0)
#define MSG_ERR_NO_HEADER				(-1)
#define MSG_ERR_HEADER_LEN_NOT_MATCH	(-2)
#define MSG_ERR_PACKET_LEN_NOT_MATCH	(-3)
#define MSG_ERR_CHKSUM_UNMATCH			(-4)
#define MSG_ERR_NO_EOP					(-5)
#define MSG_ERR_DATA_FORMAT				(-6)
#define	MSG_ERR_BUFFER_OVERFLOW			(-7)

typedef struct {
  	uint8_t 	SOM;
	uint8_t 	RFU;
  	uint16_t 	SIZE;
  	uint8_t		TYPE;
  	uint8_t		CHK;
}st_MsgHeader;
typedef union
{
	st_MsgHeader pack;
	uint8_t		array[6];
}un_MsgHeader;

typedef struct {
  	uint8_t 	SOP;
  	uint8_t		RPRT;
  	uint8_t		CHK;
  	uint8_t 	EOP;
}st_PktRprt;
typedef union
{
	st_PktRprt 	pack;
	uint8_t		array[6];
}un_PktRprt;

typedef struct {
  	uint8_t 	SOP;
	uint8_t		RPRTREQ;
  	uint16_t	TOTAL;
  	uint16_t	CURRENT;
	uint16_t	VALIDNUM;
	uint8_t		PAYLOAD[DATA_PKT_DATA_NUM];
	uint8_t		CHK;
  	uint8_t 	EOP;
}st_PktData;
typedef union
{
	st_PktData 	pack;
	uint8_t		array[DATA_PKT_DATA_NUM + 10];
}un_PktData;

// extern uint8_t* SProto_pktDistributeArr[2];
extern uint8_t SProto_rxBuffer[SPROTO_RB_SIZE];
extern uint16_t SProto_bufferWriteIdx;

// uint16_t PktHandler_PktData(un_PktData *src, uint8_t *dst);
// int8_t SProto_IsValidMsg();
// void SProto_PktSend_RPRT(uint8_t rprtValue);
// void SProto_PktSend_DATA(uint8_t *srcStart, uint32_t len);
// int8_t SProto_TLEReader(char*merged, char**lines, uint16_t size);
void SProto_UART_to_I2C_Passthrough(void);

#endif
