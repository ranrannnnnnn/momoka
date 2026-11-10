#include <main.h>

#include "led.h"

static uint16_t LEDGPIO_PIN_X[2] = { GPIO_PIN_3, GPIO_PIN_4 };


// ---------------Init---------------
void LED_on_method(LED_enum led) {
    HAL_GPIO_WritePin(GPIOA, LEDGPIO_PIN_X[led], GPIO_PIN_RESET);
}

void LED_off_method(LED_enum led) {
    HAL_GPIO_WritePin(GPIOA, LEDGPIO_PIN_X[led], GPIO_PIN_SET);
}