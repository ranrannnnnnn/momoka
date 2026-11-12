#include <main.h>

#include "common/args_define.h"
#include "hardware/led/led.h"
#include "hardware/key/key.h"
#include "hardware/oled/oled.h"

// hardware
Key key0;
Key key1;
Key key2;

void hardware_init(void) {
    LED_on_method(LED0);
    Key_hardwareInit();
    Key_init(&key0, KeyType_KEY0);
    Key_init(&key1, KeyType_KEY1);
    Key_init(&key2, KeyType_KEY2);
    OLED_Init();
}

void hardware_initFinalize(void) {  // 初始化后调用 用于启动外设或清理工作
    Key_hardwareFinalize();
}

void initComplete(void) {
    LED_on_method(LED1);



    LED_off_method(LED0);
}

int appMain(void) {
    hardware_init();
    hardware_initFinalize();
    initComplete();

    while (1) {
    }
}
