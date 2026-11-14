#include <main.h>

#include "common/args_define.h"
#include "hardware/led/led.h"
#include "hardware/key/key.h"
#include "hardware/oled/oled.h"
#include "hardware/motor/motor.h"
#include "common/args_listener/args_listener.h"
#include "hardware/motor/motor_encoder.h"
#include "connectivity/i2c/i2c.h"

// hardware
Key key0;
Key key1;
Key key2;

void hardware_init(void) {
    LED_on_method(LED0);
    Key_init(&key0, KeyType_KEY0);
    Key_init(&key1, KeyType_KEY1);
    Key_init(&key2, KeyType_KEY2);

    Motor_harwareInit();
    Pid_hardwareInit();
    ArgListener_init();
    Key_hardwareInit();
    Encoder_hardwareInit();
    I2C_hardwareInit_method();

    OLED_Init();
    Motor_init();
}

void hardware_initFinalize(void) {  // 初始化后调用 用于启动外设或清理工作
    Key_hardwareFinalize();
    Motor_hardwareFinalize();
    Encoder_hardwareFinalize();
}

void initComplete(void) {
    LED_on_method(LED1);
    // Motor_statusCheck_method();
    LED_off_method(LED0);
}

int appMain(void) {
    hardware_init();
    hardware_initFinalize();
    initComplete();

    while (1) {
        Key_mainLoopCallFunction();
        ArgListner_mainLoopFunction();
    }
}
