#pragma once

#include <stdint.h>

typedef struct {
    char* ptr;
    uint8_t length;
}String;

#define String_length_getter(String_length_getter_self) (String_length_getter_self->length)
#define String_ptr_getter(String_ptr_getter_self) (String_ptr_getter_self->ptr)