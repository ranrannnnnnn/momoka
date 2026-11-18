#include "error_handle.h"

#include "../hardware/led/led.h"
#include "../hardware/oled/oled.h"

uint8_t errorRaise = 0;

// ---------------Method---------------
void ErrorHandle_raise_method(ErrorCode errorCode) {
    errorRaise = 1;
    LED_on_method(LED1);
    switch (errorCode) {
        case ENCODER_REVERSED: {
            OLED_clear_method();
            OLED_showStringForCharArray_method(0, 1, "Encoder wiring");
            OLED_showStringForCharArray_method(1, 1, "is reversed");
            break;
        }
        case ENCODER_NOT_CONNECTED: {
            OLED_clear_method();
            OLED_showStringForCharArray_method(0, 1, "ENCODER NOT CONNECTED");
            break;
        }
    }
}
