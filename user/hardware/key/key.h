#pragma once

#include <stdint.h>

typedef enum {
    KeyStatus_RELEASED = 0U,
    KeyStatus_PRESS_DEBOUNCE,
    KeyStatus_PRESSED,
    KeyStatus_RELEASE_DEBOUNCE
}KeyStatus_enum;

typedef enum {
    KeyType_KEY0 = 0U,
    KeyType_KEY1,
    KeyType_KEY2
}KeyType_enum;

typedef struct {
    KeyType_enum keyType;
    uint8_t keySampleFlag;
    uint8_t keySampleCount;
    KeyStatus_enum keyStatus;
}Key;

void Key_init(Key *self, KeyType_enum keyType);
void Key_hardwareInit(void);
void Key_hardwareFinalize(void);
void Key_updateKeyStatus_method(Key* self);
void Key_keyStatus_setter(Key *self, uint8_t status);
void Key_TIM4_interruptCallFunction(void);