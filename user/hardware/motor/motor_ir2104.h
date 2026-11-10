#pragma once

typedef enum {
    IR2104_0 = 0U, IR2104_1
}IR2104ENUM;

void Ir2104_close_method(IR2104ENUM ir2104);
void Ir2104_open_method(IR2104ENUM ir2104);