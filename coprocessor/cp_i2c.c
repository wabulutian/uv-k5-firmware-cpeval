#include "coprocessor/cp_i2c.h"

void CP_I2C_Write(uint16_t Address, const void *pBuffer, uint16_t len)
{
	I2C_Start();
	I2C_Write(CP_I2C_ADDR_DEVICE << 1);
	I2C_Write((uint8_t)(Address & 0x00FF));
	I2C_Write((uint8_t)((Address >> 8) & 0x00FF));
	I2C_WriteBuffer(pBuffer, len);
    
	I2C_Stop();
}

void CP_I2C_Read(uint16_t Address, void *pBuffer, uint16_t len)
{
	I2C_Start();
	I2C_Write(CP_I2C_ADDR_DEVICE << 1);
	I2C_Write((uint8_t)(Address & 0x00FF));
	I2C_Write((uint8_t)((Address >> 8) & 0x00FF));

	I2C_Start();
	I2C_Write((CP_I2C_ADDR_DEVICE << 1) + 1);
	I2C_ReadBuffer(pBuffer, len);

	I2C_Stop();
}