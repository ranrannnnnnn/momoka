#pragma once

#include <stdint.h>

typedef void (*pidAdjustFunc)(uint16_t);
typedef float (*sampleFunc)(void);
typedef uint16_t (*getSettingFunc)(void);

typedef struct {
    uint16_t ts;  // 采样时间 单位ms

    float r; // 设定值

    float kp;
    float ki;

    float kd;
    float dAlpha;
    float yBefore;
    float dState;

    uint16_t setMax, setMin;
    float eiMax, eiMin;

    pidAdjustFunc pidAdjustFunc;
    sampleFunc sampleFunc;
    getSettingFunc getSettingFunc;

    float eBefore;

    float uAcc;

    uint8_t fristRun;
}Pid;

void Pid_hardwareInit(void);
void Pid_hardwareInitFinalize(void);
void Pid_init(Pid* self, uint16_t ts, float r, float kp, float ki, float kd, uint16_t setMax, uint16_t setMin,
              float tf, pidAdjustFunc pidAdjustFunc, sampleFunc sampleFunc,
              getSettingFunc getSettingFunc);
void Pid_start_method(Pid* self);
void Pid_stop_method(Pid* self);
void Pid_r_setter(Pid* self, uint16_t r);
void Pid_TIM4_interruptCallFunction(void);
