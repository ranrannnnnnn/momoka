#include "args_listener.h"
#include "../../hardware/oled/oled.h"
#include "../../hardware/motor/motor.h"
#include "../../hardware/motor/motor_encoder.h"

#define ARGSNUM 3

static void ArgListener_registerForGetter_method(getterFunction func, char* argName, uint8_t len);
static uint16_t ArgListener_getDataForGetter_method(const ArgListen* listen);
static uint16_t ___Motor_motorDirection_getter(void);
static uint16_t ___Motor_motorRpm_getter(void);

// ---------------Args---------------c
static char argName_rpm[3] = {'R', 'P', 'M'};
static char argName_pwm[3] = {'P', 'W', 'M'};
static char argName_direction[3] = {'D', 'I', 'T'};

static ArgListen agrsManagerList[ARGSNUM];
static volatile uint8_t cnt = 0;
volatile static uint16_t oledUpdateFlag = 0;

// ---------------Init---------------
void ArgListener_init(void) {
    ArgListener_registerForGetter_method(Motor_motorPwm_getter, argName_pwm, 3);
    ArgListener_registerForGetter_method(___Motor_motorDirection_getter, argName_direction, 3);
    ArgListener_registerForGetter_method(___Motor_motorRpm_getter, argName_rpm, 3);
    cnt = 0;
    for (uint8_t i = 0; i < ARGSNUM; i++) {
        OLED_showString_method(i + 1, 1, &agrsManagerList[i].argName);
    }
}

// ---------------Method---------------

static uint16_t ___Motor_motorDirection_getter(void) {
    return (uint16_t)Motor_motorDirection_getter();
}

static uint16_t ___Motor_motorRpm_getter(void) {
    return (uint16_t)(Encoder_rpm_getter() / 50);
}

static void ArgListener_registerForGetter_method(getterFunction func, char* argName, uint8_t len) {
    agrsManagerList[cnt].ptr = (uint32_t)func;
    agrsManagerList[cnt].argName.ptr = argName;
    agrsManagerList[cnt].argName.length = len;
    agrsManagerList[cnt].flag = 1;
    cnt++;
}

static uint16_t ArgListener_getDataForGetter_method(const ArgListen* listen) {
    return ((getterFunction)listen->ptr)();
}

// ---------------interruptCallFunction---------------
void ArgListener_TIM4_interruptCallFunction(void) {
    if (cnt >= 20) {
        cnt = 0;
        oledUpdateFlag = 1;
    }
    else {
        cnt++;
    }
}

// ---------------mainLoopCallFunction---------------
void ArgListner_mainLoopFunction(void) {
    if (oledUpdateFlag) {
        for (uint8_t i = 0; i < ARGSNUM; i++) {
            if (agrsManagerList[i].flag) {
                OLED_showNum_method(i + 1, agrsManagerList[i].argName.length + 1,
                    ArgListener_getDataForGetter_method(&agrsManagerList[i]));
            }
        }
        oledUpdateFlag = 0;
    }
}
