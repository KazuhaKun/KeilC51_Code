#include <REGX52.H>
#include "delay.h"

unsigned char Key_KeyNumber;

unsigned char Key(void)
{
    unsigned char Temp = 0;
    Temp = Key_KeyNumber;
    Key_KeyNumber = 0;
    return Temp;
}
/*
    *@brief 获取独立按键键码
    *@param 无
    *@retval 按下按键的键码，范围：1-4，无按键按下返回0
*/
unsigned char Key_GetState()
{
    unsigned char KeyNumber = 0;

    if(P3_1 == 0){KeyNumber = 1;}
    if(P3_0 == 0){KeyNumber = 2;}
    if(P3_2 == 0){KeyNumber = 3;}
    if(P3_3 == 0){KeyNumber = 4;}

    return KeyNumber;
}

void Key_Loop(void)
{
    static unsigned char NowState,LastState;
    LastState = NowState;
    NowState = Key_GetState();
    if (LastState != 0 && NowState == 0)
    {
        Key_KeyNumber = LastState;
    }
    // if (LastState == 1 && NowState == 0)
    // {
    //     Key_KeyNumber = 1;
    // }
    // if (LastState == 2 && NowState == 0)
    // {
    //     Key_KeyNumber = 2;
    // }
    // if (LastState == 3 && NowState == 0)
    // {
    //     Key_KeyNumber = 3;
    // }
    // if (LastState == 4 && NowState == 0)
    // {
    //     Key_KeyNumber = 4;
    // }
}