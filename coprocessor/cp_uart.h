#ifndef		__CP_UART_H__
#define		__CP_UART_H__

#include "coprocessor/simpleProtocol/simpleProtocol.h"
#include "driver/uart.h"
#include "bsp/dp32g030/uart.h"
#include "bsp/dp32g030/syscon.h"
#include "bsp/dp32g030/dma.h"
#include "external/printf/printf.h"
#include <string.h>

#define ORIGINAL_UART_RX_BUFFER         (UART_DMA_Buffer)
#define CP_UART_RX_BUFFER               (SProto_rxBuffer)


void CP_UART_Init();
void CP_UART_DeInit();

#endif