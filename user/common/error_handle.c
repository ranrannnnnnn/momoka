#include "error_handle.h"

#include "../hardware/led/led.h"
#include "../hardware/oled/oled.h"

#include <main.h>

uint8_t errorRaise = 0;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

// ---------------Method---------------
void ErrorHandle_raise_method(ErrorCode errorCode) {
    errorRaise = 1;
    LED_on_method(LED1);
    switch (errorCode) {
        case ENCODER_REVERSED: {
            OLED_clear_method();
            OLED_showStringForCharArray_method(0, 1, "ENCODER_REVERSED");
            break;
        }
        case ENCODER_NOT_CONNECTED: {
            OLED_clear_method();
            OLED_showStringForCharArray_method(0, 1, "ENCODER_NOT_CONNECTED");
            break;
        }
        case MOTOR_LOCKED: {
            OLED_clear_method();
            OLED_showStringForCharArray_method(0, 1, "MOTOR_LOCKED");
        }
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    HAL_TIM_Base_Stop(&htim1);
    HAL_TIM_Base_Stop(&htim2);
    HAL_TIM_Base_Stop(&htim3);
    HAL_TIM_Base_Stop(&htim4);
    HAL_TIM_OC_Stop(&htim1, TIM_CHANNEL_1 | TIM_CHANNEL_2 | TIM_CHANNEL_3 | TIM_CHANNEL_4);
    HAL_TIM_OC_Stop(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2 | TIM_CHANNEL_3 | TIM_CHANNEL_4);
    HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_1 | TIM_CHANNEL_2 | TIM_CHANNEL_3 | TIM_CHANNEL_4);
    HAL_TIM_OC_Stop(&htim4, TIM_CHANNEL_1 | TIM_CHANNEL_2 | TIM_CHANNEL_3 | TIM_CHANNEL_4);
}
