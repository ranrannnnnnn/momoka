#pragma once

#include <stdint.h>

void Encoder_hardwareInit(void);
void Encoder_hardwareFinalize(void);
void Encoder_sampleUpdateFlag_setter(uint8_t data);
float Encoder_rpm_getter(void);
uint8_t Encoder_sampleUpdateFlag_getter(void);
// void Encoder_EXTI15_10_interruptCallFunction(void);
void Encoder_TIM4_interruptCallFunction(void);
// void Encoder_mainLoopCallFunction(void);
