#pragma once

typedef enum {
    ENCODER_REVERSED = 0U,
    ENCODER_NOT_CONNECTED,
    MOTOR_LOCKED
}ErrorCode;

void ErrorHandle_raise_method(ErrorCode errorCode);
