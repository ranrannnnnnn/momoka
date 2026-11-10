#pragma once

typedef enum {
    LED0 = 0U,
    LED1
}LED_enum;

void LED_on_method(LED_enum led);
void LED_off_method(LED_enum led);