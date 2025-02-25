#include <REGX52.H>
#include <INTRINS.H>
#include "Timer0.h"
#include "delay.h"
#include "Key.h"
// #include "DS1302.h"

unsigned char KeyNum,LEDMode;
unsigned char Mode;
unsigned char LEDNum;
unsigned char Hour, Min, Sec;
unsigned char LEDFlashFlag = 0;

//数码管
unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,0x00,0x40};
//LED
unsigned char LEDCode[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

//数码管显示函数
void Nixie(unsigned char Location, unsigned char Num)
{
    P2_2 = Location & 0x01;
    P2_3 = (Location >> 1) & 0x01;
    P2_4 = (Location >> 2) & 0x01;
    P0 = NixieCode[Num];
    Delay(1);
    P0 = 0x00;
}

void TimeUpdate()
{
	if(Sec > 59)
	{
		Sec = 0;
		Min++;
		if(Min > 59)
		{
			Min = 0;
			Hour++;
			if(Hour > 23)
			{
				Hour = 0;
			}
		}
	}
}
void LED_ShowTime()
{
	Nixie(7,Hour/10);
	Nixie(6,Hour%10);
	Nixie(4,Min/10);
	Nixie(3,Min%10);
	Nixie(1,Sec/10);
	Nixie(0,Sec%10);
}

void main()
{
	// unsigned char i;
	Timer0Init();
	Hour = 1;
	Min = 15;
	Sec = 55;
	while(1)
	{
		KeyNum = Key();
		if(KeyNum == 1)
		{
			Hour = 1;
			Min = 15;
			Sec = 55;
			Mode = 1;
		}
		TimeUpdate();
		switch(Mode)
		{
			case 0:	//显示学号
			{
				Nixie(5,1);
				Nixie(4,9);
				Nixie(3,2);
				Nixie(2,0);
				P2 = LEDCode[LEDNum];
				break;
			}
			case 1:
			{
				Nixie(2,11);
				Nixie(5,11);
				LED_ShowTime();
				P2 = 0xFF;
				if(Hour == 1 && Min == 16 && Sec == 0)
				{
					Mode = 2;
				}
				break;
			}
			case 2:
			{
				if(LEDFlashFlag == 0) P2 = 0xFF;
				if(LEDFlashFlag == 1) P2 = LEDCode[1];
				// Nixie(2,11);
				// Nixie(5,11);
				// LED_ShowTime();
				if(Hour >= 1 && Min >= 16 && Sec >= 3)
				{
					Nixie(2,11);
					Nixie(5,11);
					LED_ShowTime();
				}
				break;
			}
		}
	}
}

void Timer0_Routine() interrupt 1
{
	static unsigned int T0Count;
	static unsigned int T0Count1;
	TL0 = 0x66; 
	TH0 = 0xFC;
	T0Count++;
	if(T0Count > 4000 && Mode == 0) //4000ms
	{
		T0Count = 0;
		// if(Mode == 0) P2 = 0xFF;
		Mode = 1;
	}

	if(T0Count % 400 == 0 && Mode == 0)
	{
		LEDNum++;
		if(LEDNum > 7) LEDNum = 0;
	}

	if(T0Count > 1000 && Mode == 1)
	{
		Sec++;
		T0Count = 0;
	}
	if(Mode == 2)
	{
		T0Count1++;
	}
	if(T0Count > 2000 && Mode == 2)
	{
		Sec++;
		T0Count = 0;
	}
	if(T0Count1 > 500 && Mode == 2)
	{
		LEDFlashFlag = !LEDFlashFlag;
		T0Count1 = 0;
	}
}