#ifndef __MATRIXLED_H__
#define __MATRIXLED_H__

void _74HC595_WriteByte(unsigned char Byte);
void MatrixLED_ShowColumn(unsigned int Column,Data);
void MatrixLED_Init();

#endif