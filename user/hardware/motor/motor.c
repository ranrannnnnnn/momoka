#include "motor.h"

#include "motor_ir2104.h"
#include "stm32f1xx_hal.h"
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
    // Pid_init(&pid0, 1, 0, 2, 6, 0.05f, 3059, 539, 30, 50, );  TODO
}

void Motor_harwareInit(void) {
    // TODO
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
    // 编码器数值回馈  TODO
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
    motor.gear = MotorGear_GEAR3;
    Pid_r_setter(&motor.pid, motor.gear);
    Pid_start_method(&motor.pid);
    // 启动编码器  TODO
}

void Motor_stop_method(void) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    Pid_stop_method(&motor.pid);
    // 编码器监测是否停止 可以堵塞主线程  TODO
}

void Motor_updateGear_method(void) {
    Pid_r_setter(&motor.pid, motor.gear);
}

void Motor_changeDirection_method(void) {
    Motor_stop_method();
    // 停稳后return  TODO
    motor.direction = motor.direction == MotorDirection_Right ? MotorDirection_Left : MotorDirection_Right;
}

void Motor_pwmOutPut_method(uint16_t pwm) {  // TODO PID传入参数之一
    if (motor.direction == MotorDirection_Right) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm);
    }
    else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm);
    }
    motor.pwm = pwm;
}

// ---------------Getter---------------
MotorStatus Motor_motorStatus_getter(void) {
    return motor.status;
}

MotorGear Motor_motorGear_getter(void) {
    return motor.gear;
}

MotorDirection Motor_motorDirection_getter(void) {
    return motor.direction;
}

uint16_t Motor_motorPwm_getter(void) {
    return motor.pwm;
}

// ---------------Setter---------------
void Motor_motorStatus_setter(MotorStatus status) {
    motor.status = status;
}

void Motor_motorDirection_setter(MotorGear gear) {
    motor.gear = gear;
    Motor_updateGear_method();
}

void Motor_motorGear_setter(MotorDirection direction) {
    motor.direction = direction;
}
