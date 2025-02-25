#include <REGX52.H>
//#include "LCD1602.h"
//#include "DS18B20.h"
#include "delay.h"
#include "Key.h"
#include "Timer0.h"
//#include "AT24C02.h"
#include "Buzzer.h"
#include "nixie.h"
#include "XPT2046.h"



// sbit Light_Green_A = P2^0;
// sbit Light_Yellow_A = P2^1;
// sbit Light_Red_A = P2^2;
// sbit Light_Green_B = P2^4;
// sbit Light_Yellow_B = P2^6;
// sbit Light_Red_B = P2^7;
sbit Light_Green_A = P1^0;
sbit Light_Yellow_A = P1^1;
sbit Light_Red_A = P1^2;
sbit Light_Green_B = P1^3;
sbit Light_Yellow_B = P1^4;
sbit Light_Red_B = P1^5;

sbit Buzzer = P2^5;

unsigned char status;
unsigned char FlashFlag_Light_Yellow = 0;
unsigned char FlashFlag_Buzzer = 0;
unsigned char KeyNum;
unsigned char CountDown;
unsigned int ADValue;


// Nixie Show CountDown
void Nixie_CountDown(unsigned char num)
{
	unsigned char i,j;
	i = num/10;
	j = num%10;
	// Nixie(1,i);
	// Nixie(2,j);
	Nixie_SetBuf(1,i);
	Nixie_SetBuf(2,j);
}

void main()
{
	Timer0_Init();
	while(1)
	{
		KeyNum = Key();
		if(KeyNum == 1) status = 4;
		if(KeyNum == 2) status = 5;
		switch(status)
		{
			case 0:		//A Green B Red
			{	//Road A
				Light_Red_A = 1;
				Light_Green_A = 0;
				//Road B
				Light_Yellow_B = 1;
				Light_Red_B = 0;
				//CountDown
				Nixie_CountDown(CountDown);
				break;
			}
			case 1:		// A Yellow
			{	//Road A
				Light_Green_A = 1;
				Light_Yellow_A = FlashFlag_Light_Yellow;
				//Road B:Do nothing
				//Buzzer
				if(FlashFlag_Light_Yellow) Buzzer_Time(100);
				//CountDown
				Nixie_CountDown(CountDown);
				break;
			}
			case 2:		//B Green A Red
			{	//Road A
				Light_Yellow_A = 1;
				Light_Red_A = 0;
				//Road B
				Light_Red_B = 1;
				Light_Green_B = 0;
				//CountDown
				Nixie_CountDown(CountDown);
				break;
			}
			case 3:		// B Yellow
			{	//Road B
				Light_Green_B = 1;
				// if(FlashFlag_Light_Yellow == 0) Light_Yellow_B = 0;
				// if(FlashFlag_Light_Yellow == 1) Light_Yellow_B = 1;
				Light_Yellow_B = FlashFlag_Light_Yellow;
				//Road A:Do nothing
				//Buzzer
				if(FlashFlag_Light_Yellow) Buzzer_Time(100);
				//CountDown
				Nixie_CountDown(CountDown);
				break;
			}
			case 4:		//Emergency
			{	//Road A
				Light_Green_A = 1;
				Light_Yellow_A = 1;
				Light_Red_A = 0;
				//Road B
				Light_Green_B = 1;
				Light_Yellow_B = 1;
				Light_Red_B = 0;
				break;
			}
			case 5:		//Night Mode
			{	//Road A
				Light_Green_A = 1;
				Light_Red_A = 1;
				Light_Yellow_A = FlashFlag_Light_Yellow;
				//Road B
				Light_Green_B = 1;
				Light_Red_B = 1;
				Light_Yellow_B = FlashFlag_Light_Yellow;
				break;
			}
		}
	}
}


void Timer0_Routine() interrupt 1
{
	static unsigned int KeyCount, LightCount, FlashCount, T0Count;
	TL0 = 0x66; 
	TH0 = 0xFC;
	KeyCount++;
	LightCount++;
	FlashCount++;
	T0Count++;
	//Nixie Loop
	if(T0Count >= 2){T0Count = 0;Nixie_Loop();}
	//GR Scan
	if(T0Count >= 2){T0Count = 0;ADValue=XPT2046_ReadAD(XPT2046_VBAT);if(ADValue < 190) status = 5;if(ADValue > 160) status = 0;}
	//Key Detect
	if(KeyCount >= 20){KeyCount = 0;Key_Loop();}
	//Flash Flag
	if (FlashCount >= 500){FlashCount = 0;FlashFlag_Light_Yellow = ~FlashFlag_Light_Yellow;}
	// //Buzzer Control
	// if(FlashCount >= 500 && (status == 1 || status == 3)) {FlashCount = 0;FlashFlag_Buzzer = ~FlashFlag_Buzzer;}
	//Light Control
	if(status == 0 && LightCount >= 5000){LightCount = 0;status = 1;}
	if(status == 1 && LightCount >= 2000){LightCount = 0;status = 2;}
	if(status == 2 && LightCount >= 5000){LightCount = 0;status = 3;}
	if(status == 3 && LightCount >= 2000){LightCount = 0;status = 0;}
	if(status == 4 && LightCount >= 10000){LightCount = 0;status = 0;}
	//LED Num
	if(LightCount%1000 == 1) {
		if(status == 0 || status == 2) CountDown = 5-(LightCount/1000);
		if(status == 1 || status == 3) CountDown = 2-(LightCount/1000);
	}
}
