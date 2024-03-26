#include "coprocessor/cp_i2c.h"

void CP_I2C_Write(uint8_t Address, const void *pBuffer, uint16_t len)
{
	I2C_Start();
	I2C_Write(CP_I2C_ADDR_DEVICE << 1);
	I2C_Write(Address);
	I2C_WriteBuffer(pBuffer, len);
    
	I2C_Stop();

	SYSTEM_DelayMs(10);
}

void CP_I2C_Read(uint8_t Address, void *pBuffer, uint8_t len)
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