#include "motor_encoder.h"
#include "../../config/motorConfig.h"

#include <main.h>

#include "motor.h"
#include "../../common/math/math_base.h"

// ---------------Args---------------
extern TIM_HandleTypeDef htim2;

static volatile uint8_t sampleCnt = 0;
static volatile  uint8_t sampleCnt1 = 0;
static volatile uint8_t sampleCnt2 = 0;

static float rpm = 0.0f;
static int16_t cnt_TIM2 = 0;

// static uint8_t dwtBeforeFlag = 0;
// static uint32_t dwtBefore;
// static uint32_t us;
// static uint16_t cntTemp;

static volatile uint8_t sampleUpdateFlag = 0;

// ---------------Init---------------
void Encoder_hardwareInit(void) {
    // // DWT计时初始化
    // CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // DWT->CYCCNT = 0;
    // DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // // PB3, PA15EXTI配置
    // __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
    // GPIO_InitTypeDef gi = {0};
    // gi.Pin  = GPIO_PIN_15;
    // gi.Mode = GPIO_MODE_IT_RISING;
    // gi.Pull = GPIO_NOPULL;
    // HAL_GPIO_Init(GPIOA, &gi);
    // __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
    // HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    // HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Encoder_hardwareFinalize(void) {
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
}

// ---------------Method---------------
// static inline uint32_t Encoder_getDwtCounter_method(void) {
//     return DWT->CYCCNT;
// }

static inline void Encoder_clearTim2Counter_method(void) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

static void Encoder_motorStatusUpdate_method(void) {
    if (cnt_TIM2 < 0) {
        Motor_motorDirection_setter(MotorDirection_Left);
    }
    else if (cnt_TIM2 > 0) {
        Motor_motorDirection_setter(MotorDirection_Right);
    }
    switch (Motor_motorStatus_getter()) {
        case MotorStatus_RUNNING: {
            if (cnt_TIM2 != 0) {
                break;
            }
            sampleCnt++;
            Motor_motorStatus_setter(MotorStatus_STOPPED_DEBOUNCE);
            break;
        }
        case MotorStatus_RUNNING_DEBOUNCE: {
            if (cnt_TIM2 == 0) {
                sampleCnt++;
                sampleCnt1++;
                if (sampleCnt1 >= 5) {
                    Motor_motorStatus_setter(MotorStatus_STOPPED);
                    sampleCnt1 = 0;
                    sampleCnt = 0;
                }
                break;
            }
            sampleCnt++;
            if (sampleCnt >= 2) {
                sampleCnt = 0;
                sampleCnt1 = 0;
                Motor_motorStatus_setter(MotorStatus_RUNNING);
            }
            break;
        }
        case MotorStatus_STOPPED_DEBOUNCE: {
            if (cnt_TIM2 != 0) {
                sampleCnt++;
                sampleCnt1++;
                if (sampleCnt1 >= 5) {
                    Motor_motorStatus_setter(MotorStatus_RUNNING);
                    sampleCnt1 = 0;
                    sampleCnt = 0;
                }
                break;
            }
            sampleCnt++;
            if (sampleCnt >= 20) {
                sampleCnt = 0;
                sampleCnt1 = 0;
                Motor_motorStatus_setter(MotorStatus_STOPPED);
            }
            break;
        }
        case MotorStatus_STOPPED: {
            if (cnt_TIM2 == 0) {
                break;
            }
            sampleCnt++;
            Motor_motorStatus_setter(MotorStatus_STOPPED_DEBOUNCE);
            break;
        }
    }
}

// ---------------Setter---------------
void Encoder_sampleUpdateFlag_setter(uint8_t data) {
    sampleUpdateFlag = data;
}

// ---------------Getter---------------
float Encoder_rpm_getter(void) {
    return rpm;
}

uint8_t Encoder_sampleUpdateFlag_getter(void) {
    return sampleUpdateFlag;
}

// ---------------InterruptCallFunction---------------
// void Encoder_EXTI15_10_interruptCallFunction(void) {
//     if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15)) {
//         if (dwtBeforeFlag) {
//             dwtBeforeFlag = 0;
//             if (MathBase_absoluteValue_tool((int32_t)cntTemp - (int32_t)__HAL_TIM_GetCounter(&htim2)) != 4) {
//                 return;
//             }
//             uint32_t cycles = Encoder_getDwtCounter_method() - dwtBefore;
//             us = cycles / (SystemCoreClock / 1000000U);
//             sampleUpdateFlag = 1;
//         }
//         else {
//             dwtBefore = Encoder_getDwtCounter_method();
//             dwtBeforeFlag = 1;
//             cntTemp = __HAL_TIM_GetCounter(&htim2);
//         }
//         __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
//     }
// }

void Encoder_TIM4_interruptCallFunction(void) {
    sampleCnt2++;
    if (sampleCnt2 > 10) {
        cnt_TIM2 = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
        rpm = MathBase_absoluteValue_tool(cnt_TIM2) / 7 * 100;
        Encoder_clearTim2Counter_method();
        Encoder_motorStatusUpdate_method();
    }
}

// ---------------MainLoopCallFunction---------------
// void Encoder_mainLoopCallFunction(void) {
//     if (sampleUpdateFlag) {
//         rpm = 1000000.0f / (DDR * (float)us);
//     }
// }

