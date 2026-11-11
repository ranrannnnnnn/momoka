#include "key.h"
#include "../../common/bit.h"
#include "../../config/keyConfig.h"
#include "../../hardware/motor/motor.h"

#include <main.h>

typedef void (*keyPressedHandle)(void);

static void Key_pressedHandle_key0(void);
static void Key_pressedHandle_key1(void);
static void Key_pressedHandle_key2(void);
static void Key_releasedHandle_key0(void);
static void Key_releasedHandle_key1(void);
static void Key_releasedHandle_key2(void);

extern TIM_HandleTypeDef htim4;

// ---------------Args---------------
// 按键状态变化回调函数
static const keyPressedHandle callbackFunctionTable[] = {
    Key_pressedHandle_key0, Key_pressedHandle_key1,
    Key_pressedHandle_key2, Key_releasedHandle_key0,
    Key_releasedHandle_key1, Key_releasedHandle_key2
};
static const uint16_t GPIO_PIN_X[] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2
};
static Key* keyManagerList[KEYNUMBER] = {NULL};
static uint8_t timerSampleCount = 0;

// ---------------Init---------------
void Key_init(Key* self, KeyType_enum keyType) {
    self->keyType = keyType;
    self->keyStatus = KeyStatus_PRESS_DEBOUNCE;
    self->keySampleCount = 0;
    self->keySampleFlag = 0;
    for (uint8_t i = 0; i < KEYNUMBER; i++) {
        if (keyManagerList[i] == NULL) {
            keyManagerList[i] = self;
        }
    }
}

void Key_hardwareInit(void) {
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim4);
}

void Key_hardwareFinalize(void) {
    HAL_TIM_Base_Start(&htim4);
}

// ---------------Method---------------
/**
 * 按键状态更新 5ms一周期
 */
void Key_updateKeyStatus_method(Key* self) {
    switch (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_X[self->keyType])) {
        case GPIO_PIN_RESET: {
            switch (self->keyStatus) {
                case KeyStatus_PRESS_DEBOUNCE: {
                    self->keySampleCount++;
                    if (self->keySampleCount == KEYSAMPLECOUNT) {
                        self->keySampleCount = 0;
                        Key_keyStatus_setter(self, KeyStatus_PRESSED);
                        break;
                    }
                    break;
                }
                case KeyStatus_RELEASE_DEBOUNCE: {
                    if (self->keySampleFlag) {
                        Key_keyStatus_setter(self, KeyStatus_PRESSED);
                        self->keySampleCount = 0;
                        self->keySampleFlag = 0;
                        break;
                    }
                    self->keySampleFlag = 1;
                    break;
                }
                case KeyStatus_PRESSED: { break; }
                case KeyStatus_RELEASED: {
                    Key_keyStatus_setter(self, KeyStatus_PRESS_DEBOUNCE);
                    self->keySampleCount++;
                    break;
                }
                default: break;
            }
            break;
        }
        case GPIO_PIN_SET: {
            switch (self->keyStatus) {
                case KeyStatus_PRESS_DEBOUNCE: {
                    if (self->keySampleFlag) {
                        Key_keyStatus_setter(self, KeyStatus_RELEASED);
                        self->keySampleCount = 0;
                        self->keySampleFlag = 0;
                        break;
                    }
                    self->keySampleFlag = 1;
                    break;
                }
                case KeyStatus_RELEASE_DEBOUNCE: {
                    self->keySampleCount++;
                    if (self->keySampleCount == KEYSAMPLECOUNT) {
                        self->keySampleCount = 0;
                        Key_keyStatus_setter(self, KeyStatus_RELEASED);
                        break;
                    }
                    break;
                }
                case KeyStatus_PRESSED: {
                    Key_keyStatus_setter(self, KeyStatus_RELEASE_DEBOUNCE);
                    self->keySampleCount++;
                    break;
                }
                case KeyStatus_RELEASED: { break; }
                default: break;
            }
            break;
        }
    }
}

// ---------------Setter---------------
void Key_keyStatus_setter(Key *self, uint8_t status) {
    self->keyStatus = status;
    switch (status) {
        case KeyStatus_PRESSED: {
            callbackFunctionTable[self->keyType]();
            break;
        }
        case KeyStatus_RELEASED: {
            callbackFunctionTable[3 + self->keyType]();
            break;
        }
        default: break;
    }
}

// ---------------CallbackFunction---------------
static void Key_pressedHandle_key0(void) {
}

static void Key_pressedHandle_key1(void) {
}

static void Key_pressedHandle_key2(void) {
}

static void Key_releasedHandle_key0(void) {
}

static void Key_releasedHandle_key1(void) {
}

static void Key_releasedHandle_key2(void) {
}

// ---------------interruptCallFunction---------------
void Key_TIM4_interruptCallFunction(void) {
    if (timerSampleCount < 4) {
        timerSampleCount++;
        return;
    }
    timerSampleCount = 0;
    for (uint8_t i = 0; i < KEYNUMBER; i++) {
        if (keyManagerList[i] == NULL) { continue; }
        Key_updateKeyStatus_method(keyManagerList[i]);
    }
}
