#pragma once

// -----------------------------------
// 基础运算库
// -----------------------------------

/**
  * @brief  整数的指数运算, 使用宏实现泛型
  * @param  base 基底
  * @param  exponent 指数
  * @param result 指针形式 用于存储返回值
  * @retval None
  */
#define MathBase_pow_tool(base, exponent, result)  do{\
    __typeof__(base) mathPowBase = base;\
    __typeof__(exponent) mathPowExponent = exponent;\
    *result = 1;\
    while (mathPowExponent > 0)\
    {\
        if (mathPowExponent & 1)\
        {\
            (*result) *= mathPowBase;\
        }\
        mathPowBase *= mathPowBase;\
        mathPowExponent >>= 1;\
    }\
}while (0)

/**
  * @brief  max()函数
  * @param result 指针形式 用于存储返回值
  * @retval None
  */
#define MathBase_max_tool(a, b, result) do{\
    __typeof__(a) mathMaxA = a;\
    __typeof__(b) mathMaxB = b;\
    if (mathMaxA > mathMaxB) { *result = mathMaxA; }\
    else { *result = mathMaxB; }\
}while (0)
