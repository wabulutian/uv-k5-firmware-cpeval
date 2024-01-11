#include "driver/rtc.h"

void RTC_Init(void)
{
    uint32_t pre = 0;
    uint32_t preRound = 32767;
    uint32_t preDecimal = 0;
    uint32_t prePeriod = 0;
    
    RTC_RTC_PRE = ((preRound << RTC_RTC_PRE_PRE_ROUND_SHIFT) & RTC_RTC_PRE_PRE_ROUND_MASK) | 
            ((preDecimal << RTC_RTC_PRE_PRE_DECIMAL_SHIFT) & RTC_RTC_PRE_PRE_DECIMAL_MASK) | 
            ((prePeriod << RTC_RTC_PRE_PRE_PERIOD_SHIFT) & RTC_RTC_PRE_PRE_PERIOD_MASK);
}

uint8_t RTC_SetDateTime(uint8_t yr_dec, uint8_t yr, uint8_t mon_dec, uint8_t mon, uint8_t day_dec, uint8_t day,
                        uint8_t hr_dec, uint8_t hr, uint8_t min_dec, uint8_t min, uint8_t sec_dec, uint8_t sec)
{
    uint32_t dr = 0;
    uint32_t tr = 0;

    dr =    ((yr_dec <<     RTC_RTC_DR_BCD_YEAR_DEC_SHIFT) &    RTC_RTC_DR_BCD_YEAR_DEC_MASK) | 
            ((yr <<         RTC_RTC_DR_BCD_YEAR_SHIFT) &        RTC_RTC_DR_BCD_YEAR_MASK) | 
            ((mon_dec <<    RTC_RTC_DR_BCD_MONTH_DEC_SHIFT) &    RTC_RTC_DR_BCD_MONTH_DEC_MASK) | 
            ((mon <<        RTC_RTC_DR_BCD_MONTH_SHIFT) &        RTC_RTC_DR_BCD_MONTH_MASK) | 
            ((day_dec <<    RTC_RTC_DR_BCD_DATE_DEC_SHIFT) &    RTC_RTC_DR_BCD_DATE_DEC_MASK) | 
            ((day <<        RTC_RTC_DR_BCD_DATE_SHIFT) &        RTC_RTC_DR_BCD_DATE_MASK);

    tr =    ((hr_dec <<     RTC_RTC_TR_BCD_HOUR_DEC_SHIFT) &    RTC_RTC_TR_BCD_HOUR_DEC_MASK) | 
            ((hr <<         RTC_RTC_TR_BCD_HOUR_SHIFT) &        RTC_RTC_TR_BCD_HOUR_MASK) | 
            ((min_dec <<    RTC_RTC_TR_BCD_MIN_DEC_SHIFT) &    RTC_RTC_TR_BCD_MIN_DEC_MASK) |
            ((min <<        RTC_RTC_TR_BCD_MIN_SHIFT) &        RTC_RTC_TR_BCD_MIN_MASK) | 
            ((sec_dec <<    RTC_RTC_TR_BCD_SEC_DEC_SHIFT) &    RTC_RTC_TR_BCD_SEC_DEC_MASK) | 
            ((sec <<        RTC_RTC_TR_BCD_SEC_SHIFT) &        RTC_RTC_TR_BCD_SEC_MASK);

    RTC_RTC_DR = dr;
    RTC_RTC_TR = tr;
    if((RTC_RTC_IF & RTC_RTC_IF_TIME_ERR_MASK) >> RTC_RTC_IF_TIME_ERR_SHIFT) return 1;
    return 0;
}

void RTC_LoadDateTime(void)
{
    RTC_RTC_CFG |= RTC_RTC_CFG_LOAD_EN_BITS_ENABLE;
    SYSTICK_DelayUs(1000);
    RTC_RTC_CFG |= RTC_RTC_CFG_LOAD_EN_BITS_DISABLE;
}

void RTC_Enable(void)
{
    RTC_RTC_CFG |= 0x1;
}

void RTC_ReadDateTime(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *sec)
{
    uint8_t _yr_dec =   (uint32_t)(RTC_RTC_TSDR & RTC_RTC_TSDR_YEAR_DEC_MASK) >> RTC_RTC_TSDR_YEAR_DEC_SHIFT;
    uint8_t _yr =       (uint32_t)(RTC_RTC_TSDR & RTC_RTC_TSDR_YEAR_MASK) >>     RTC_RTC_TSDR_YEAR_SHIFT;
    uint8_t _mon_dec =  (uint32_t)(RTC_RTC_TSDR & RTC_RTC_TSDR_MONTH_DEC_MASK) >> RTC_RTC_TSDR_MONTH_DEC_SHIFT;
    uint8_t _mon =      (uint32_t)(RTC_RTC_TSDR & RTC_RTC_TSDR_MONTH_MASK) >>     RTC_RTC_TSDR_MONTH_SHIFT;
    uint8_t _day_dec =  (uint32_t)(RTC_RTC_TSDR & RTC_RTC_TSDR_DATE_DEC_MASK) >> RTC_RTC_TSDR_DATE_DEC_SHIFT;
    uint8_t _day =      (uint32_t)(RTC_RTC_TSDR & RTC_RTC_TSDR_DATE_MASK) >>     RTC_RTC_TSDR_DATE_SHIFT;

    uint8_t _hr_dec =   (uint32_t)(RTC_RTC_TSTR & RTC_RTC_TSTR_HOUR_DEC_MASK) >> RTC_RTC_TSTR_HOUR_DEC_SHIFT;
    uint8_t _hr =       (uint32_t)(RTC_RTC_TSTR & RTC_RTC_TSTR_HOUR_MASK) >>     RTC_RTC_TSTR_HOUR_SHIFT;
    uint8_t _min_dec =  (uint32_t)(RTC_RTC_TSTR & RTC_RTC_TSTR_MIN_DEC_MASK) >> RTC_RTC_TSTR_MIN_DEC_SHIFT;
    uint8_t _min =      (uint32_t)(RTC_RTC_TSTR & RTC_RTC_TSTR_MIN_MASK) >>     RTC_RTC_TSTR_MIN_SHIFT;
    uint8_t _sec_dec =  (uint32_t)(RTC_RTC_TSTR & RTC_RTC_TSTR_SEC_DEC_MASK) >> RTC_RTC_TSTR_SEC_DEC_SHIFT;
    uint8_t _sec =      (uint32_t)(RTC_RTC_TSTR & RTC_RTC_TSTR_SEC_MASK) >>     RTC_RTC_TSTR_SEC_SHIFT;

    *year = _yr_dec * 10 + _yr;
    *month = _mon_dec * 10 + _mon;
    *day = _day_dec * 10 + _day;
    *hour = _hr_dec * 10 + _hr;
    *minute = _min_dec * 10 + _min;
    *sec = _sec_dec * 10 + _sec;
}