#pragma once

#include "../../common/pid/pid.h"

#include <stdint.h>

typedef enum {
    MotorStatus_RUNNING = 0U,
    MotorStatus_RUNNING_DEBOUNCE,
    MotorStatus_STOPPED_DEBOUNCE,
    MotorStatus_STOPPED
}MotorStatus;

typedef enum {
    MotorGear_GEAR3 = 0U,
    MotorGear_GEAR2,
    MotorGear_GEAR1
}MotorGear;

typedef enum {
    MotorDirection_Right = 0U,
    MotorDirection_Left
}MotorDirection;

typedef struct {
    MotorStatus status;
    MotorDirection direction;
    MotorGear gear;
    Pid pid;
    uint16_t pwm;
}Motor;

void Motor_init(void);
void Motor_harwareInit(void);
void Motor_hardwareFinalize(void);
void Motor_statusCheck_method(void);
void Motor_start_method(void);
void Motor_stop_method(void);
void Motor_updateGear_method(void);
void Motor_changeDirection_method(void);
MotorStatus Motor_motorStatus_getter(void);
MotorGear Motor_motorGear_getter(void);
MotorDirection Motor_motorDirection_getter(void);
uint16_t Motor_motorPwm_getter(void);
void Motor_motorStatus_setter(MotorStatus motorStatus);
void Motor_motorDirection_setter(MotorDirection direction);
void Motor_motorGear_setter(MotorGear gear);
