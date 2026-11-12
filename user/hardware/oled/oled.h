#pragma once

#include "../../common/string.h"

void OLED_Init(void);
void OLED_showString_method(uint8_t line, uint8_t column, String* string);
void OLED_clear_method(void);
void OLED_showNum_method(uint8_t line, uint8_t column, uint32_t number);
