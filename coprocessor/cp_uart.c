#include "coprocessor/cp_uart.h"

/**
 * Set baudrate to 600000. Set DMA dst to Uart_Rx_Buffer
*/
void CP_UART_Init()
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
	DMA_CH0->MDADDR = (uint32_t)(uintptr_t)CP_UART_RX_BUFFER;

	DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;
	UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
}

/**
 * Restore baudrate and DMA setting
*/
void CP_UART_DeInit()
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

    DMA_CH0->MDADDR = (uint32_t)(uintptr_t)ORIGINAL_UART_RX_BUFFER;

    DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;
	UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
}