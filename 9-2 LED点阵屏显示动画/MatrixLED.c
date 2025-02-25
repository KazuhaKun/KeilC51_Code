#include <REGX52.H>
#include "delay.h"

sbit RCK = P3^5;		//RCLK
sbit SCK = P3^6;		//SRCLK
sbit SER = P3^4;

#define MATRIX_LED_PORT	P0
/*
	*@brief	74HC595写入一个字节
	*@param	Byte	要写入的字节
	*@retval	无
*/
void _74HC595_WriteByte(unsigned char Byte)
{
	unsigned char i;
	RCK = 0;
	SCK = 0;
	for(i=0;i<8;i++)
	{
		SER = Byte >> 7;
		Byte <<= 1;
		SCK = 1;
		SCK = 0;
	}
	RCK = 1;
	RCK = 0;
}

/* 江协写法
void _74HC595_WriteByte(unsigned char Byte) {
	unsigned char i;
	RCK = 0;
	SCK = 0;
	for(i=0;i<8;i++)
	{
		SER = Byte & (0x80 >> i);
		SCK = 1;
		SCK = 0;
	}
	RCK = 1;
	RCK = 0;
} 
*/

// unsigned char ColumnData[9] = {0x00,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

/*
	*@brief	LED点阵屏显示某一列&某几列
	*@param	Column	(0~8)显示的某一列数 (0~0xff)显示的某几列数 均为从左到右
	*@param	Data	选择列现实的数据，高位在上，1亮0灭
	*@retval	无
*/

void MatrixLED_ShowColumn(unsigned int Column,Data)
{
	if (Column <= 8)
	{
		_74HC595_WriteByte(Data);
		// MATRIX_LED_PORT = ~ ColumnData[Column];
		MATRIX_LED_PORT = ~ (0x80 >> Column);
		Delay(1);
		MATRIX_LED_PORT = 0xFF;
	}
	if (Column > 8)
	{
		unsigned char x,y,z;
		_74HC595_WriteByte(Data);
		x = (Column & 0x0F) << 4 | (Column & 0xF0) >> 4;
		y = (Column & 0x33) << 2 | (Column & 0xCC) >> 2;
		z = (Column & 0x55) << 1 | (Column & 0xAA) >> 1;
		MATRIX_LED_PORT = z;
		Delay(1);
		MATRIX_LED_PORT = 0xFF;
	}
	if (Column > 0xff) return;
}

void MatrixLED_Init()
{
    SCK = 0;
	RCK = 0;
}