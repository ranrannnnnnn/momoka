#include "string.h"

// ---------------Init---------------
/**
 * @param length 字符串长度
 * @param ptr 字符串首位指针, 注意字符串内存必须在.data段分配(字符串内容在编译前确定), 不支持堆分配
 */
void String_init(String* self, uint8_t length, char* ptr) {
    self->length = length;
    self->ptr = ptr;
}
