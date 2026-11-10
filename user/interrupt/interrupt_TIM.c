#include <main.h>

#include "../hardware/key/key.h"
#include "../common/pid/pid.h"

void TIM4_Interrupt(void) {
    Key_TIM4_interruptCallFunction();
    Pid_TIM4_interruptCallFunction();
}