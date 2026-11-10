#pragma once

#include "../../common/pid/pid.h"

#include <stdint.h>

typedef enum {
    MotorStatus_RUNNING = 0U,
    MotorStatus_LOCKED_DEBOUNCE,
    MotorStatus_LOCKED,
    MotorStatus_BRAKE,
    MotorStatus_STOPPED_DEBOUNCE,
    MotorStatus_STOPPED,
}MotorStatus;

typedef enum {
    MotorGear_GEAR3 = 300U,
    MotorGear_GEAR2 = 200U,
    MotorGear_GEAR1 = 150U
}MotorGear;

typedef enum {
    MotorDirection_Right = 0U,
    MotorDirection_Left
}MotorDirection;

typedef struct {
    MotorStatus motorStatus;
    MotorDirection direction;
    MotorGear gear;
    Pid pid;
}Motor;