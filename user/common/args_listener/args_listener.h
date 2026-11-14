#pragma once

#include <stdint.h>
#include "../string.h"

typedef struct {
    uint32_t ptr;
    uint8_t flag;
    String argName;
}ArgListen;

typedef uint16_t (*getterFunction)(void);

void ArgListener_init(void);
void ArgListener_TIM4_interruptCallFunction(void);
void ArgListner_mainLoopFunction(void);
