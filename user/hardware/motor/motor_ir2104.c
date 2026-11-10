#include "motor_ir2104.h"

#include <main.h>

// ---------------Args---------------
static uint16_t PINX[2] = { GPIO_PIN_5, GPIO_PIN_6 };

// ---------------Method---------------
void Ir2104_close_method(IR2104ENUM ir2104) {
    HAL_GPIO_WritePin(GPIOA, PINX[ir2104], GPIO_PIN_RESET);
}

void Ir2104_open_method(IR2104ENUM ir2104) {
    HAL_GPIO_WritePin(GPIOA, PINX[ir2104], GPIO_PIN_SET);
}
