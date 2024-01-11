#ifndef		__SATPASS_UART_H__
#define		__SATPASS_UART_H__

#include "satpass_ref.h"
#include "driver/uart.h"
#include "bsp/dp32g030/uart.h"
#include "bsp/dp32g030/syscon.h"
#include "bsp/dp32g030/dma.h"
#include "external/printf/printf.h"
#include <string.h>

#define DMA_INDEX(x, y) (((x) + (y)) % sizeof(UART_DMA_Buffer))
#define	DATA_PKT_DATA_NUM	512

#define	SATPASS_MSG_HEADER_SOM	0xAB
#define	SATPASS_MSG_PKT_SOP		0xAA
#define	SATPASS_MSG_PKT_EOP		0xEE

#define SATPASS_MSG_TYPE_RPRT	0x0
#define	SATPASS_MSG_TYPE_DATA	0x1

#define SATPASS_UART_DMA_SIZE       800

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

extern uint8_t Satpass_Uart_rxBuffer[SATPASS_UART_DMA_SIZE];
extern uint8_t* Satpass_Uart_pktDistributeArr[2];

void Satpass_UART_Init();
void Satpass_UART_DeInit();
uint16_t PktHandler_PktData(un_PktData *src, uint8_t *dst);
int8_t Satpass_UART_IsValidMsg();
void PktSend_RPRT(uint8_t rprtValue);

#endif