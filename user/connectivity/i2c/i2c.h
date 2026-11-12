#pragma once

#include <stdint.h>

typedef struct {
    uint8_t address;
}I2C;

void I2C_init(I2C* self, uint8_t address);
void I2C_sendByte_method(uint8_t byte);
void I2C_writeRegister_method(I2C* self, uint8_t regAddr, uint8_t regData);
