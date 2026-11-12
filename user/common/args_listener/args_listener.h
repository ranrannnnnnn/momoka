#pragma once

#include <stdint.h>
#include "../string.h"

typedef struct {
    uint32_t ptr;
    uint8_t flag;
    String argName;
}ArgListen;