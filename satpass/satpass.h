#ifndef		__SATPASS_H__
#define		__SATPASS_H__

#include "satpass_ref.h"
#include "satpass_uart.h"
#include "satpass_ui.h"
#include "satpass_def.h"
#include "board.h"
#include "driver/uart.h"
#include "driver/rtc.h"
#include "driver/gpio.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/keyboard.h"
#include "ui/helper.h"
#include "helper/battery.h"
#include "helper/measurements.h"
#include "bsp/dp32g030/uart.h"
#include "bsp/dp32g030/irq.h"
#include "bsp/dp32g030/syscon.h"
#include "bsp/dp32g030/dma.h"
#include "bsp/dp32g030/gpio.h"
#include "app/finput.h"
#include "radio.h"
#include "misc.h"
#include "settings.h"
#include "ARMCM0.h"
#include <stdbool.h>
#include <stdint.h>
#define RSSITEST

#ifdef RSSITEST

typedef struct
{
    uint8_t code;
    uint8_t mask;
    uint8_t ascii;
}st_cwLUT;

static st_cwLUT cwLUT[] = {
    {.code = 0b01,		.mask = 0b11,		.ascii = 'A'},
	{.code = 0b1000,	.mask = 0b1111,		.ascii = 'B'},
	{.code = 0b1010,	.mask = 0b1111,		.ascii = 'C'},
	{.code = 0b100,		.mask = 0b111,		.ascii = 'D'},
	{.code = 0b0,		.mask = 0b1,		.ascii = 'E'},
	{.code = 0b0010,	.mask = 0b1111,		.ascii = 'F'},
	{.code = 0b110,		.mask = 0b111,		.ascii = 'G'},
	{.code = 0b0000,	.mask = 0b1111,		.ascii = 'H'},
	{.code = 0b00,		.mask = 0b11,		.ascii = 'I'},
	{.code = 0b0111,	.mask = 0b1111,		.ascii = 'J'},
	{.code = 0b101,		.mask = 0b111,		.ascii = 'K'},
	{.code = 0b0100,	.mask = 0b1111,		.ascii = 'L'},
	{.code = 0b11,		.mask = 0b11,		.ascii = 'M'},
	{.code = 0b10,		.mask = 0b11,		.ascii = 'N'},
	{.code = 0b111,		.mask = 0b111,		.ascii = 'O'},
	{.code = 0b0110,	.mask = 0b1111,		.ascii = 'P'},
	{.code = 0b1101,	.mask = 0b1111,		.ascii = 'Q'},
	{.code = 0b010,		.mask = 0b111,		.ascii = 'R'},
	{.code = 0b000,		.mask = 0b111,		.ascii = 'S'},
	{.code = 0b1,		.mask = 0b1,		.ascii = 'T'},
	{.code = 0b001,		.mask = 0b111,		.ascii = 'U'},
	{.code = 0b0001,	.mask = 0b1111,		.ascii = 'V'},
	{.code = 0b011,		.mask = 0b111,		.ascii = 'W'},
	{.code = 0b1001,	.mask = 0b1111,		.ascii = 'X'},
	{.code = 0b1011,	.mask = 0b1111,		.ascii = 'Y'},
	{.code = 0b1100,	.mask = 0b1111,		.ascii = 'Z'},
	{.code = 0b11111,	.mask = 0b11111,	.ascii = '0'},
	{.code = 0b01111,	.mask = 0b11111,	.ascii = '1'},
	{.code = 0b00111,	.mask = 0b11111,	.ascii = '2'},
	{.code = 0b00011,	.mask = 0b11111,	.ascii = '3'},
	{.code = 0b00001,	.mask = 0b11111,	.ascii = '4'},
	{.code = 0b00000,	.mask = 0b11111,	.ascii = '5'},
	{.code = 0b10000,	.mask = 0b11111,	.ascii = '6'},
	{.code = 0b11000,	.mask = 0b11111,	.ascii = '7'},
	{.code = 0b11100,	.mask = 0b11111,	.ascii = '8'},
	{.code = 0b11110,	.mask = 0b11111,	.ascii = '9'},
	{.code = 0b010101,	.mask = 0b111111,	.ascii = '.'},
	{.code = 0b110011,	.mask = 0b111111,	.ascii = ','},
	{.code = 0b111000,	.mask = 0b111111,	.ascii = ':'},
	{.code = 0b001100,	.mask = 0b111111,	.ascii = '?'},
	{.code = 0b100001,	.mask = 0b111111,	.ascii = '-'},
	{.code = 0b10010,	.mask = 0b11111,	.ascii = '/'},
	{.code = 0b10110,	.mask = 0b11111,	.ascii = '('},
	{.code = 0b101101,	.mask = 0b111111,	.ascii = ')'},
	{.code = 0b10001,	.mask = 0b11111,	.ascii = '='},
	{.code = 0b01010,	.mask = 0b11111,	.ascii = '+'}
};

#endif

void APP_RunSatpass();
#endif