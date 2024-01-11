#include "satpass_uart.h"

uint8_t* Satpass_Uart_pktDistributeArr[2] = {0};
uint8_t Satpass_Uart_rxBuffer[SATPASS_UART_DMA_SIZE];
uint16_t Satpass_Uart_dmaReadIndex = 0;
uint8_t Satpass_Uart_pktTxBuffer[256];

/**
 * Set baudrate to 600000. Set DMA dst to Satpass_Uart_Rx_Buffer
*/
void Satpass_UART_Init()
{
    uint32_t Delta;
	uint32_t Positive;
	uint32_t Frequency;

	UART1->CTRL = (UART1->CTRL & ~UART_CTRL_UARTEN_MASK) | UART_CTRL_UARTEN_BITS_DISABLE;
    DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_DISABLE;

	Delta = SYSCON_RC_FREQ_DELTA;
	Positive = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_SIG_MASK) >> SYSCON_RC_FREQ_DELTA_RCHF_SIG_SHIFT;
	Frequency = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_DELTA_MASK) >> SYSCON_RC_FREQ_DELTA_RCHF_DELTA_SHIFT;
	if (Positive) {
		Frequency += 48000000U;
	} else {
		Frequency = 48000000U - Frequency;
	}

	UART1->BAUD = Frequency / 610203U;

	// Set dst addr
	DMA_CH0->MDADDR = (uint32_t)(uintptr_t)Satpass_Uart_rxBuffer;

	DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;
	UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
}

/**
 * Restore baudrate and DMA setting
*/
void Satpass_UART_DeInit()
{
    uint32_t Delta;
	uint32_t Positive;
	uint32_t Frequency;

	UART1->CTRL = (UART1->CTRL & ~UART_CTRL_UARTEN_MASK) | UART_CTRL_UARTEN_BITS_DISABLE;
    DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_DISABLE;

	Delta = SYSCON_RC_FREQ_DELTA;
	Positive = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_SIG_MASK) >> SYSCON_RC_FREQ_DELTA_RCHF_SIG_SHIFT;
	Frequency = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_DELTA_MASK) >> SYSCON_RC_FREQ_DELTA_RCHF_DELTA_SHIFT;
	if (Positive) {
		Frequency += 48000000U;
	} else {
		Frequency = 48000000U - Frequency;
	}

	UART1->BAUD = Frequency / 39053U;

    DMA_CH0->MDADDR = (uint32_t)(uintptr_t)ORIGINAL_UART_DMA_DST;

    DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;
	UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
}

uint16_t PktHandler_PktData(un_PktData *src, uint8_t *dst)
{
	uint16_t numToTransmit = src->pack.VALIDNUM;
	uint16_t currentPkt = src->pack.CURRENT;
	uint16_t totalPkt = src->pack.TOTAL;
	uint16_t addrOffset = currentPkt * DATA_PKT_DATA_NUM;
	for (uint16_t i = 0; i < numToTransmit; i ++)
	{
		memcpy((uint8_t*)(dst + addrOffset), src->pack.PAYLOAD, numToTransmit);
	}

	return(totalPkt - currentPkt);
}

int8_t Satpass_UART_IsValidMsg()
{
	uint16_t DmaLength;
	uint16_t MsgRxLength;
	uint8_t CHK;

	DmaLength = DMA_CH0->ST & 0xFFFU;
	un_MsgHeader header;
	while (1) {
		if (Satpass_Uart_dmaReadIndex == DmaLength) {
			return -1;
		}

		// Find Header1
		while (Satpass_Uart_dmaReadIndex != DmaLength &&
           	Satpass_Uart_rxBuffer[Satpass_Uart_dmaReadIndex] != SATPASS_MSG_HEADER_SOM) {
				Satpass_Uart_dmaReadIndex = DMA_INDEX(Satpass_Uart_dmaReadIndex, 1);
		}
		// Current Satpass_Uart_dmaReadIndex is pointing SOM if noting wrong
		// Calculate remain data length
		if (Satpass_Uart_dmaReadIndex < DmaLength) {
			MsgRxLength = DmaLength - Satpass_Uart_dmaReadIndex;
		} else {
			MsgRxLength = (DmaLength + sizeof(Satpass_Uart_rxBuffer)) - Satpass_Uart_dmaReadIndex;
		}
		// Check valid Header length
		if (MsgRxLength < 6) {
      		return -3;
    	}
		// Copy Header data
		for (int i = 0; i < 6; i ++)
		{
			header.array[i] = Satpass_Uart_rxBuffer[DMA_INDEX(Satpass_Uart_dmaReadIndex, i)];
		}
		// Check valid Msg length
		if (MsgRxLength < header.pack.SIZE) {
      		return -4;
    	}
		if (Satpass_Uart_rxBuffer[DMA_INDEX(Satpass_Uart_dmaReadIndex, header.pack.SIZE - 1)] != SATPASS_MSG_PKT_EOP) {
			Satpass_Uart_dmaReadIndex = DmaLength;	//Reset Satpass_Uart_dmaReadIndex
      		return -5;
    	}

		// Checksum
		CHK = 0;
		for (uint16_t i = 0; i < header.pack.SIZE - 2; i ++) {
			CHK += Satpass_Uart_rxBuffer[DMA_INDEX(Satpass_Uart_dmaReadIndex, i)];
		}
		if (CHK != Satpass_Uart_rxBuffer[DMA_INDEX(Satpass_Uart_dmaReadIndex, header.pack.SIZE - 2)]) {
			Satpass_Uart_dmaReadIndex = DmaLength;	//Reset Satpass_Uart_dmaReadIndex
      		return CHK;
		}

		// Transfer pkt to union
		uint8_t *packArr = Satpass_Uart_pktDistributeArr[header.pack.TYPE];
		for (int i = 0; i < header.pack.SIZE; i ++)
		{
			packArr[i] = Satpass_Uart_rxBuffer[DMA_INDEX(Satpass_Uart_dmaReadIndex, i + 6)];
		}
		Satpass_Uart_dmaReadIndex = DmaLength;	//Reset Satpass_Uart_dmaReadIndex
		return header.pack.TYPE;
	}
}

void PktSend_RPRT(uint8_t rprtValue)
{
	un_MsgHeader header;
	un_PktRprt pkt;
	header.pack.TYPE = SATPASS_MSG_TYPE_RPRT;
	header.pack.SIZE = 10;
	header.pack.SOM = SATPASS_MSG_HEADER_SOM;
	pkt.pack.SOP = SATPASS_MSG_PKT_SOP;
	pkt.pack.RPRT = rprtValue;
	pkt.pack.EOP = SATPASS_MSG_PKT_EOP;
	pkt.pack.CHK = 0;
	for (int i = 0; i < 6; i ++)
	{
		pkt.pack.CHK += header.array[i];
	}
	for (int i = 0; i < 2; i ++)
	{
		pkt.pack.CHK += pkt.array[i];
	}
	memcpy(Satpass_Uart_pktTxBuffer, header.array, 6);
	memcpy(&Satpass_Uart_pktTxBuffer[6], pkt.array, 4);
	UART_Send(Satpass_Uart_pktTxBuffer, 10);
}