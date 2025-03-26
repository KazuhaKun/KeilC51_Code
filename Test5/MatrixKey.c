#include <REGX52.H>
#include "delay.h"

unsigned char MatrixKey_KeyNumber;

unsigned char MatrixKey(void)
{
    unsigned char Temp = 0;
    Temp = MatrixKey_KeyNumber;
    MatrixKey_KeyNumber = 0;
    return Temp;
}
/*
 *@brief: 矩阵键盘读取按键键码
 *@param: 无
 *@retval: KeyNumber 按下按键的键码值
           如果按键按下不放，程序会停留在词函数，松手的一瞬间，法案会按键键码；没有按键按下时，返回0
 */
unsigned char MatrixKey_GetState()
{
    unsigned char KeyNumber = 0;

    P1 = 0xFF;
    P1_3 = 0;
    if (P1_7 == 0) {KeyNumber = 1;}
    if (P1_6 == 0) {KeyNumber = 5;}
    if (P1_5 == 0) {KeyNumber = 9;}
    if (P1_4 == 0) {KeyNumber = 13;}

    P1 = 0xFF;
    P1_2 = 0;
    if (P1_7 == 0) {KeyNumber = 2;}
    if (P1_6 == 0) {KeyNumber = 6;}
    if (P1_5 == 0) {KeyNumber = 10;}
    if (P1_4 == 0) {KeyNumber = 14;}

    P1 = 0xFF;
    P1_1 = 0;
    if (P1_7 == 0) {KeyNumber = 3;}
    if (P1_6 == 0) {KeyNumber = 7;}
    if (P1_5 == 0) {KeyNumber = 11;}
    if (P1_4 == 0) {KeyNumber = 15;}

    P1 = 0xFF;
    P1_0 = 0;
    if (P1_7 == 0) {KeyNumber = 4;}
    if (P1_6 == 0) {KeyNumber = 8;}
    if (P1_5 == 0) {KeyNumber = 12;}
    if (P1_4 == 0) {KeyNumber = 16;}
    
    return KeyNumber;
}

void MatrixKey_Loop(void)
{
    static unsigned char NowState,LastState;
	  LastState=NowState;				//按键状态更新
	  NowState=MatrixKey_GetState();		//获取当前按键状态
	//如果上个时间点按键按下，这个时间点未按下，则是松手瞬间，以此避免消抖和松手检测
    if (LastState != 0 && NowState == 0)
    {
        MatrixKey_KeyNumber = LastState;
    }
}