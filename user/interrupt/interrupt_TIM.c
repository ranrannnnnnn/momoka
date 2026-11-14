#include "../hardware/key/key.h"
#include "../common/pid/pid.h"
#include "../hardware/motor/motor.h"
#include "../common/args_listener/args_listener.h"
#include "../hardware/motor/motor_encoder.h"

void TIM4_Interrupt(void) {
    Encoder_TIM4_interruptCallFunction();
    Key_TIM4_interruptCallFunction();
    Pid_TIM4_interruptCallFunction();
    ArgListener_TIM4_interruptCallFunction();
}

void TIM3_Interrupt(void) {
    Motor_TIM3_interruptCallFunction();
}