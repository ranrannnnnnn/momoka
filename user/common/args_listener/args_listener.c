#include "args_listener.h"
#include "../../hardware/oled/oled.h"
#include "../../hardware/motor/motor.h"

#define ARGSNUM 2

static void ArgListener_registerForGetter_method(getterFunction func, char* argName, uint8_t len);
static uint16_t ArgListener_getDataForGetter_method(const ArgListen* listen);

// ---------------Args---------------
static char argName_rpm[3] = {'R', 'P', 'M'};
static char argName_pwm[3] = {'P', 'W', 'M'};

static ArgListen agrsManagerList[ARGSNUM];
static uint8_t cnt = 0;

// ---------------Init---------------
void ArgListener_init(void) {
    ArgListener_registerForGetter_method(Motor_motorPwm_getter, argName_rpm, 3);
    // rpm TODO
    cnt = 0;
    for (uint8_t i = 0; i < ARGSNUM; i++) {
        OLED_showString_method(i + 1, 0, &agrsManagerList[i].argName);
    }
}

// ---------------Method---------------
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
    }
    else {
        cnt++;
        return;
    }
    for (uint8_t i = 0; i < ARGSNUM; i++) {
        if (agrsManagerList[i].flag) {
            OLED_showNum_method(i + 1, agrsManagerList[i].argName.length + 1,
                ArgListener_getDataForGetter_method(&agrsManagerList[i]));
        }
    }
}