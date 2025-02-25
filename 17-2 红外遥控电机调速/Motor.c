#include <REGX52.H>
#include "Timer1.h"

unsigned char Counter,Compare;

sbit Motor = P1^0;

void Motor_Init(void)
{
    Timer1_Init();
}

void Motor_SetSpeed(unsigned char speed)
{
	Compare = speed;
}

void Timer1_Routine() interrupt 3
{
	TL1 = 0xA4; 
	TH1 = 0xFF;
    Counter++;
    Counter%=100;
    if(Counter<Compare)
    {
        Motor = 1;
    } 
    else
    {
        Motor = 0;
    }
    
}