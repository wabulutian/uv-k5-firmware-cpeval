#include "coprocessor/cp_i2c.h"

void CP_I2C_Write(uint8_t Address, const void *pBuffer, uint16_t len)
{
	I2C_Start();
	I2C_Write(CP_I2C_ADDR_DEVICE << 1);
	I2C_Write(Address);
	I2C_WriteBuffer(pBuffer, len);
    
	I2C_Stop();
}

void CP_I2C_Read(uint8_t Address, void *pBuffer, uint16_t len)
{
	I2C_Start();
	I2C_Write(CP_I2C_ADDR_DEVICE << 1);
	I2C_Write(Address);

	I2C_Start();
	I2C_Write((CP_I2C_ADDR_DEVICE << 1) + 1);
	I2C_ReadBuffer(pBuffer, len);

	I2C_Stop();
}

void CP_I2C_Info(void *pBuffer)
{
    CP_I2C_Read(CP_I2C_ADDR_INFO, pBuffer, CP_I2C_SIZE_INFO);
}

void CP_I2C_RTC(void *pBuffer, uint8_t rw)
{
    if (rw == CP_I2C_DIR_READ) CP_I2C_Read(CP_I2C_ADDR_TIME, pBuffer, CP_I2C_SIZE_TIME);
    else CP_I2C_Write(CP_I2C_ADDR_TIME, pBuffer, CP_I2C_SIZE_TIME);
}

void CP_I2C_SATINFO(void *pBuffer, uint8_t index)
{
    CP_I2C_Read(CP_I2C_ADDR_SAT_INFO_START + index, pBuffer, CP_I2C_SIZE_SAT_INFO);
}

void CP_I2C_TLE(void *pBuffer, uint8_t index)
{
    CP_I2C_Write(CP_I2C_ADDR_TLE_START + index, pBuffer, CP_I2C_SIZE_TLE);
}

void CP_I2C_Site(void *pBuffer)
{
	CP_I2C_Write(CP_I2C_ADDR_QTH_LL, pBuffer, CP_I2C_SIZE_QTH_LL);
}

void CP_I2C_Freq(void *pBuffer, uint8_t index)
{
	CP_I2C_Write(CP_I2C_ADDR_FREQ_START + index, pBuffer, CP_I2C_SIZE_FREQ);
}

void CP_I2C_PASSINFO(void *pBuffer, uint8_t index)
{
    CP_I2C_Read(CP_I2C_ADDR_PASS_INFO_START + index, pBuffer, CP_I2C_SIZE_PASS_INFO);
}

