#ifndef DRIVER_RTC_H
#define DRIVER_RTC_H
#include <stdio.h>
#include "bsp/dp32g030/rtc.h"
#include "driver/systick.h"

void RTC_Init(void);
uint8_t RTC_SetDateTime(uint8_t yr_dec, uint8_t yr, uint8_t mon_dec, uint8_t mon, uint8_t day_dec, uint8_t day,
                        uint8_t hr_dec, uint8_t hr, uint8_t min_dec, uint8_t min, uint8_t sec_dec, uint8_t sec);
void RTC_ReadDateTime(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *sec);
void RTC_LoadDateTime(void);
void RTC_Enable(void);

#endif