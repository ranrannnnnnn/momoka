#include "fortLib.h"
#include "../../connectivity/i2c/i2c.h"
#include "../../common/math/math_base.h"
#include "oled.h"

#include <main.h>

static void OLED_writeCommand_method(uint8_t Command);
static void OLED_writeData_method(uint8_t data);
static void OLED_setCursor_method(const uint8_t y, const uint8_t x);
static void OLED_showChar_method(const uint8_t line, const uint8_t column, const char _char);

// ---------------Args---------------
static I2C i2c;

// ---------------Init---------------
void OLED_Init(void)
{
	uint32_t i, j;

	I2C_init(&i2c, 0x3C);

	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}

	OLED_writeCommand_method(0xAE);	//关闭显示

	OLED_writeCommand_method(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_writeCommand_method(0x80);

	OLED_writeCommand_method(0xA8);	//设置多路复用率
	OLED_writeCommand_method(0x3F);

	OLED_writeCommand_method(0xD3);	//设置显示偏移
	OLED_writeCommand_method(0x00);

	OLED_writeCommand_method(0x40);	//设置显示开始行

	OLED_writeCommand_method(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置

	OLED_writeCommand_method(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_writeCommand_method(0xDA);	//设置COM引脚硬件配置
	OLED_writeCommand_method(0x12);

	OLED_writeCommand_method(0x81);	//设置对比度控制
	OLED_writeCommand_method(0xCF);

	OLED_writeCommand_method(0xD9);	//设置预充电周期
	OLED_writeCommand_method(0xF1);

	OLED_writeCommand_method(0xDB);	//设置VCOMH取消选择级别
	OLED_writeCommand_method(0x30);

	OLED_writeCommand_method(0xA4);	//设置整个显示打开/关闭

	OLED_writeCommand_method(0xA6);	//设置正常/倒转显示

	OLED_writeCommand_method(0x8D);	//设置充电泵
	OLED_writeCommand_method(0x14);

	OLED_writeCommand_method(0xAF);	//开启显示

	OLED_clear_method();				//OLED清屏
}
// ---------------Method---------------

/**
  * @brief  OLED写命令
  * @param  command 要写入的命令
  * @retval 无
  */
static void OLED_writeCommand_method(const uint8_t command)
{
	I2C_writeRegister_method(&i2c, 0x00, command);
}

/**
  * @brief  OLED写数据
  * @param  data 要写入的数据
  * @retval 无
  */
static void OLED_writeData_method(const uint8_t data)
{
	I2C_writeRegister_method(&i2c, 0x40, data);
}

/**
  * @brief  OLED设置光标位置
  * @param  y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  x 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
static void OLED_setCursor_method(const uint8_t y, const uint8_t x)
{
	OLED_writeCommand_method(0xB0 | y);					//设置Y位置
	OLED_writeCommand_method(0x10 | ((x & 0xF0) >> 4));	//设置X位置高4位
	OLED_writeCommand_method(0x00 | (x & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED显示一个字符
  * @param  line 行位置，范围：1~4
  * @param  column 列位置，范围：1~16
  * @param  _char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
static void OLED_showChar_method(const uint8_t line, const uint8_t column, const char _char)
{      	
	uint8_t i;
	OLED_setCursor_method((line - 1) * 2, (column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_writeData_method(OLED_F8x16[_char - ' '][i]);			//显示上半部分内容
	}
	OLED_setCursor_method((line - 1) * 2 + 1, (column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_writeData_method(OLED_F8x16[_char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  line 起始行位置，范围：1~4
  * @param  column 起始列位置，范围：1~16
  * @param  string 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_showString_method(const uint8_t line, const uint8_t column, const String* string)
{
	for (uint8_t i = 0; i < String_length_getter(string); i++)
	{
		OLED_showChar_method(line, column + i, String_ptr_getter(string)[i]);
	}
}

/**
  *  OLED清屏
  */
void OLED_clear_method(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_setCursor_method(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_writeData_method(0x00);
		}
	}
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  line 起始行位置，范围：1~4
  * @param  column 起始列位置，范围：1~16
  * @param  number 要显示的数字，范围：0~4294967295
  * @retval 无
  */
void OLED_showNum_method(uint8_t line, uint8_t column, uint32_t number)
{
	uint8_t length = MathBase_numberLength_tool(number);
	for (uint8_t i = 0; i < length; i++)
	{
		OLED_showChar_method(line, column + i, number / MathBase_pow_tool(10, length - i - 1) % 10 + '0');
	}
}

// /**
//   * @brief  OLED显示数字（十进制，带符号数）
//   * @param  Line 起始行位置，范围：1~4
//   * @param  Column 起始列位置，范围：1~16
//   * @param  Number 要显示的数字，范围：-2147483648~2147483647
//   * @param  Length 要显示数字的长度，范围：1~10
//   * @retval 无
//   */
// void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
// {
// 	uint8_t i;
// 	uint32_t Number1;
// 	if (Number >= 0)
// 	{
// 		OLED_showChar_method(Line, Column, '+');
// 		Number1 = Number;
// 	}
// 	else
// 	{
// 		OLED_showChar_method(Line, Column, '-');
// 		Number1 = -Number;
// 	}
// 	for (i = 0; i < Length; i++)
// 	{
// 		OLED_showChar_method(Line, Column + i + 1, Number1 / MathBase_pow_tool(10, Length - i - 1) % 10 + '0');
// 	}
// }

// /**
//   * @brief  OLED显示数字（十六进制，正数）
//   * @param  line 起始行位置，范围：1~4
//   * @param  column 起始列位置，范围：1~16
//   * @param  number 要显示的数字，范围：0~0xFFFFFFFF
//   * @param  length 要显示数字的长度，范围：1~8
//   * @retval 无
//   */
// void OLED_showHexNum_method(const uint8_t line, const uint8_t column, uint32_t number)
// {
// 	uint8_t length = MathBase_hexLength_tool(number);
// 	for (uint8_t i = 0; i < length; i++)
// 	{
// 		uint8_t SingleNumber = number / MathBase_pow_tool(16, length - i - 1) % 16;
// 		if (SingleNumber < 10)
// 		{
// 			OLED_showChar_method(line, column + i, SingleNumber + '0');
// 		}
// 		else
// 		{
// 			OLED_showChar_method(line, column + i, SingleNumber - 10 + 'A');
// 		}
// 	}
// }

// /**
//   * @brief  OLED显示数字（二进制，正数）
//   * @param  Line 起始行位置，范围：1~4
//   * @param  Column 起始列位置，范围：1~16
//   * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
//   * @param  Length 要显示数字的长度，范围：1~16
//   * @retval 无
//   */
// void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
// {
// 	uint8_t i;
// 	for (i = 0; i < Length; i++)
// 	{
// 		OLED_showChar_method(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
// 	}
// }

