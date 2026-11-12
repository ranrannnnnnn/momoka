#include "i2c.h"

#include <main.h>

// ---------------Macro---------------
#define I2C_writeBitSCL_method(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (x))
#define I2C_writeBitSDA_method(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (x))

// ---------------Init---------------
void I2C_init(I2C* self, uint8_t address) {
    self->address = address;
}

// ---------------Method---------------
static void I2C_startSingal_method(void)
{
    I2C_writeBitSDA_method(1);
    I2C_writeBitSCL_method(1);
    I2C_writeBitSDA_method(0);
    I2C_writeBitSCL_method(0);
}

static void I2C_stopSingle_method(void)
{
    I2C_writeBitSDA_method(0);
    I2C_writeBitSCL_method(1);
    I2C_writeBitSDA_method(1);
}

void I2C_sendByte_method(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++)
    {
        I2C_writeBitSDA_method(byte & (0x80 >> i));
        I2C_writeBitSCL_method(1);
        I2C_writeBitSCL_method(0);
    }
    I2C_writeBitSCL_method(1);	//额外的一个时钟，不处理应答信号
    I2C_writeBitSCL_method(0);
}

void I2C_writeRegister_method(I2C* self, uint8_t regAddr, uint8_t regData) {
    I2C_startSingal_method();
    I2C_sendByte_method((self->address << 1));
    I2C_sendByte_method(regAddr);
    I2C_sendByte_method(regData);
    I2C_stopSingle_method();
}
