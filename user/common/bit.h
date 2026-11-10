#pragma once

#include <stdint.h>
#include "math/math_base.h"
#include "./args_define.h"

// -----------------------------------
// 位操作库
// -----------------------------------


// ---------------Tools---------------
/**
 * 位置位
 * @param a 数据指针
 * @param n 写入位, 最低位为0
 */
static inline void Bit_setBit_tool(uint8_t* a, uint8_t n) {
    *a |= (1 << n);
}

/**
 * 位清除
 * @param a 数据指针
 * @param n 写入位, 最低位为0
 */
static inline void Bit_resetBit_tool(uint8_t* a, uint8_t n) {
    *a &= ~(1 << n);
}

/**
 * 位写入
 * @param a 数据指针
 * @param n 写入位, 最低位为0
 * @param writeData 写入数据, 接受以下参数 RAN_SET, RAN_RESET
 */
static inline void Bit_writeBit_tool(uint8_t* a, uint8_t n, uint8_t writeData) {
    if (writeData != RAN_SET && writeData != RAN_RESET) { return; }
    if (writeData == RAN_SET) { Bit_setBit_tool(a, n); }
    else { Bit_resetBit_tool(a, n); }
}


/**
 * 多位写入
 * @param a 数据指针
 * @param n 数据最低位
 * @param size 数据长度
 * @param writeData 写入数据
 */
static inline void Bit_writeMultipleBits_tool(uint8_t* a, uint8_t n, uint8_t size, uint8_t writeData) {
    uint8_t temp;
    MathBase_pow_tool(2, size, &temp);
    temp -= 1; temp <<= n; temp = ~temp;
    *a &= temp;
    *a += (writeData << n);
}

/**
 * 位读取
 * @param a 数据指针
 * @param n 读取位
 * @return 读取的数据
 */
static inline uint8_t Bit_readBit_tool(const uint8_t* a, uint8_t n) {
    return ((*a >> n) & 254) ^ (*a >> n);
}

/**
 * 多位读取
 * @param a 数据指针
 * @param n 读取位
 * @param size 数据长度
 * @return 读取的数据
 */
static inline uint8_t Bit_readMultipleBits_tool(const uint8_t* a, uint8_t n, uint8_t size) {
    uint8_t temp;
    MathBase_pow_tool(2, size, &temp);
    temp -= 1; temp <<= n;
    return (*a & temp) >> n;
}