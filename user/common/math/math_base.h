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
#define MathBase_pow_tool(base, exponent)  ({\
    __typeof__(base) mathPowBase = base;\
    __typeof__(exponent) mathPowExponent = exponent;\
    __typeof__(base) mathPowresult = 1;\
    while (mathPowExponent > 0)\
    {\
        if (mathPowExponent & 1)\
        {\
            (mathPowresult) *= mathPowBase;\
        }\
        mathPowBase *= mathPowBase;\
        mathPowExponent >>= 1;\
    }\
    mathPowresult;\
})

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

#define MathBase_numberLength_tool(num) ({\
    __typeof__(num) mathnumberLengthCnt = 0;\
    while (1) {\
        if (num - MathBase_pow_tool(10, mathnumberLengthCnt) > num) {\
            break;\
        }\
        mathnumberLengthCnt++;\
    }\
    mathnumberLengthCnt;\
})

#define MathBase_absoluteValue_tool(a) ({\
    __typeof__(a) mathAbsoluteValue = a;\
    if (mathAbsoluteValue < 0) {\
        mathAbsoluteValue = -mathAbsoluteValue;\
    }\
    mathAbsoluteValue;\
})
