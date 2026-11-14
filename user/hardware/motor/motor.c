#include "motor.h"
#include "motor_ir2104.h"
#include "motor_encoder.h"
#include "../../common/pid/pid.h"

#include <main.h>

static void Motor_pwmOutput_method(uint16_t pwm);

// ---------------Args---------------
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;

static uint16_t gearAimRpmList[3] = {7500, 10000, 15000};

static volatile uint8_t MotorStatusCheckInterruptFlag = 0;
static volatile uint8_t watchDogFlag = 0;
// static volatile  uint8_t motorLockedFlag = 0;
static Motor motor;
// ---------------Init---------------
void Motor_init(void) {
    // 初始化PID实例
    // 占空比15% - 85%
    Pid_init(&motor.pid, 10, 0, 2, 6, 0.05f, 3059, 539, 50,
        Motor_pwmOutput_method, Encoder_rpm_getter, Motor_motorPwm_getter);
    motor.direction = MotorDirection_Right;
    motor.status = MotorStatus_STOPPED;
}

void Motor_harwareInit(void) {
    htim3.Init.Period = 4999;
    htim3.Init.Prescaler = 7199;
    HAL_TIM_Base_Init(&htim3);
    Ir2104_close_method(IR2104_0);
    Ir2104_close_method(IR2104_1);
}

void Motor_hardwareFinalize(void) {
    HAL_TIM_Base_Start(&htim1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
}

// ---------------Method---------------
void Motor_statusCheck_method(void) {
    __HAL_TIM_CLEAR_IT(&htim1, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start(&htim3);
    Ir2104_open_method(IR2104_0);
    Ir2104_open_method(IR2104_1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1500);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    MotorStatusCheckInterruptFlag = 1;
    while (motor.status != MotorStatus_RUNNING) {
        if (watchDogFlag)
        { break; }
    }
    MotorStatusCheckInterruptFlag = 0;
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_Base_Stop(&htim3);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    if (watchDogFlag) {
        Ir2104_close_method(IR2104_0);
        Ir2104_close_method(IR2104_1);
        while (1) {}
    }
    while (motor.status != MotorStatus_STOPPED) {}
    __HAL_TIM_CLEAR_IT(&htim1, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start(&htim3);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 1500);
    MotorStatusCheckInterruptFlag = 1;
    while (motor.status != MotorStatus_RUNNING) {
        if (watchDogFlag)
        { break; }
    }
    MotorStatusCheckInterruptFlag = 0;
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_Base_Stop(&htim3);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    if (watchDogFlag) {
        Ir2104_close_method(IR2104_0);
        Ir2104_close_method(IR2104_1);
        while (1) {}
    }
    while (motor.status != MotorStatus_STOPPED) {}
}

void Motor_start_method(void) {
    Ir2104_open_method(IR2104_0);
    Ir2104_open_method(IR2104_1);
    motor.gear = MotorGear_GEAR3;
    Pid_r_setter(&motor.pid, gearAimRpmList[motor.gear]);
    Pid_start_method(&motor.pid);
}

void Motor_stop_method(void) {
    Pid_stop_method(&motor.pid);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    motor.pwm = 0;
    htim3.Init.Period = 49999;
    htim3.Init.Prescaler = 7199;
    HAL_TIM_Base_Init(&htim3);
    MotorStatusCheckInterruptFlag = 1;
    HAL_TIM_Base_Start_IT(&htim3);
    __HAL_TIM_CLEAR_IT(&htim1, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&htim3);
    while (motor.status != MotorStatus_STOPPED) {
        if (watchDogFlag)
            { watchDogFlag = 0; break; }
    }
    MotorStatusCheckInterruptFlag = 0;
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_Base_Stop(&htim3);
}

void Motor_updateGear_method(void) {
    Pid_r_setter(&motor.pid, gearAimRpmList[motor.gear]);
}

void Motor_changeDirection_method(void) {
    Motor_stop_method();
    motor.direction = motor.direction == MotorDirection_Right ? MotorDirection_Left : MotorDirection_Right;
    Motor_start_method();
}

static void Motor_pwmOutput_method(uint16_t pwm) {
    if (motor.direction == MotorDirection_Right) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    }
    else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
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
    // if (motor.status == MotorStatus_RUNNING && status == MotorStatus_STOPPED_DEBOUNCE) {
    //     motorLockedFlag = 1;
    // }
    // else if (motorLockedFlag && status == MotorStatus_STOPPED) {
    //     motorLockedFlag = 0;
    //     Motor_stop_method();
    // }
    // else if (motorLockedFlag && status == MotorStatus_RUNNING ) {
    //     motorLockedFlag = 0;
    // }
}

void Motor_motorGear_setter(MotorGear gear) {
    motor.gear = gear;
    Motor_updateGear_method();
}

void Motor_motorDirection_setter(MotorDirection direction) {
    motor.direction = direction;
}

// ---------------interruptCallFunction---------------
void Motor_TIM3_interruptCallFunction(void) {
    if (MotorStatusCheckInterruptFlag) {
        watchDogFlag = 1;
    }
}
