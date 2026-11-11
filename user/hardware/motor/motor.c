#include "motor.h"

#include "motor_ir2104.h"
#include "stm32f1xx_hal_tim.h"
#include "../../common/pid/pid.h"


// ---------------Args---------------
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;

volatile uint8_t MotorStatusCheckInterruptFlag = 0;
static Motor motor;
// ---------------Init---------------
void Motor_init(void) {
    // 初始化PID实例
    // 占空比15% - 85%
    // Pid_init(&pid0, 1, 0, 2, 6, 0.05f, 3059, 539, 30, 50, );
}

void Motor_harwareInit(void) {

}

void Motor_hardwareFinalize(void) {
    HAL_TIM_Base_Start(&htim1);
    HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_2);
}

// ---------------Method---------------
void Motor_statusCheck_method(void) {
    __HAL_TIM_CLEAR_IT(&htim1, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start(&htim3);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1500);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    // 编码器数值回馈
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 1500);
    // 编码器数值回馈
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_Base_Stop(&htim3);
    // 刹车
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
}

void Motor_start_method(void) {
    Ir2104_open_method(IR2104_0);
    Ir2104_open_method(IR2104_1);
    Pid_r_setter(&pid0, );
    Pid_start_method(&pid0);
}

void Motor_stop_method(void) {
    
}

void Motor_changeGear_method(void) {}

void Motor_changeDirection_method(void) {}

// ---------------Getter---------------
MotorStatus Motor_motorStatus_getter(void) {}

MotorGear Motor_motorGear_getter(void) {}

MotorDirection Motor_motorDirection_getter(void) {}

// ---------------Setter---------------
void Motor_motorStatus_setter(MotorStatus motorStatus) {}

void Motor_motorDirection_setter(MotorGear motorGear) {}

void Motor_motorGear_setter(MotorDirection motorDirection) {}
