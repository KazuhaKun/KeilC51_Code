#include <REGX52.H>
#include "Timer0.h"
#include "delay.h"
#include "Key.h"
#include "nixie.h"

unsigned char Counter,Compare;
unsigned char i;

sbit DA = P2^1;


void main()
{
    // unsigned char i;
    Timer0_Init();
    Compare = 5;
    while(1)
    {
        for(i=0;i<100;i++)
        {
            Compare = i;
            Delay(10);
        }
        for(i=100;i>0;i--)
        {
            Compare = i;
            Delay(10);
        }
        
        
    }
}

void Timer0_Routine() interrupt 1
{
	TL0 = 0xA4; 
	TH0 = 0xFF;
    Counter++;
    Counter%=100;
    if(Counter<Compare)
    {
        DA = 1;
    } 
    else
    {
        DA = 0;
    }
    
}