#include <REGX52.H>
#include "nixie.h"
#include "Timer0.h"
#include "Key.h"
#include "Buzzer.h"
#include "XPT2046.h"
#include "MatrixKey.h"


sbit Buzzer = P2^5;

unsigned char mode = 0;
bit BuzzerFlag = 0;
bit BeepFlag = 0;
bit LEDFlag = 0;
unsigned char LEDLocal = 0;
unsigned char LEDNum=0;
unsigned char KeyNum;

unsigned int LEDCounter = 0;
unsigned int ADValue;
unsigned int CountSet = 32000;


void KeyProc(){
    static unsigned char lastKey = 0;
    KeyNum = MatrixKey();
    
    if(KeyNum == 1 && lastKey != 1){
        if(mode == 0 || mode == 2) mode = 1;
        else mode = 2;
        
        if(mode == 1) {
            LEDLocal = 0;
            LEDNum = 0;
        }
    }
    
    if(KeyNum == 3 && lastKey != 3){
        BuzzerFlag = ~BuzzerFlag;
    }

	if(KeyNum == 10 && lastKey != 10){
		if(CountSet == 32000) CountSet = 64000;
		else CountSet = 32000;
	}
	lastKey = KeyNum;

}

void Nixie_Init(){
	Nixie_SetBuf(1,2,0);
	Nixie_SetBuf(2,4,0);
	Nixie_SetBuf(3,2,0);
	Nixie_SetBuf(4,1,0);
	Nixie_SetBuf(5,1,0);
	Nixie_SetBuf(6,9,0);
	Nixie_SetBuf(7,2,0);
	Nixie_SetBuf(8,0,0);
}

void main()
{
	Timer0_Init();
	while(1)
	{
		switch(mode){
			case 0:
			{
				Nixie_Init();
				break;
			}
			case 1:
			{
				Nixie_SetBuf(1,0,1);
				Nixie_SetBuf(2,5,0);
				Nixie_SetBuf(3,ADValue/100%10,0);
				Nixie_SetBuf(4,ADValue/10%10,0);
				Nixie_SetBuf(5,ADValue%10,0);
				Nixie_SetBuf(6,10,0);
				Nixie_SetBuf(7,LEDNum/10%10,0);
				Nixie_SetBuf(8,LEDNum%10,0);
				if(BeepFlag == 1 && BuzzerFlag == 1) Buzzer_Time(100);
				break;
			}
			case 2:
			{
				Nixie_SetBuf(1,1,1);
				Nixie_SetBuf(2,0,0);
				Nixie_SetBuf(3,10,0);
				Nixie_SetBuf(4,10,0);
				Nixie_SetBuf(5,10,0);
				Nixie_SetBuf(6,10,0);
				Nixie_SetBuf(7,10,0);
				Nixie_SetBuf(8,10,0);
				break;
			}
		}
	}
}

void Timer0_Routine() interrupt 1
{
    static unsigned int Count0,NixieCount,AD_Set;
    TL0 = 0x66; 
    TH0 = 0xFC;
    Count0++;
    NixieCount++;
    if(mode == 1) {
        LEDCounter++;
        if(LEDCounter >= (CountSet/AD_Set)) {  // 500ms
            LEDCounter = 0;
			
			P2 = ~(0x01 << (7-LEDLocal));

            if(++LEDLocal == 8) LEDLocal = 0;
            if(++LEDNum == 100) LEDNum = 0;
            if(LEDLocal==0) BeepFlag = 1;
			else BeepFlag = 0;
        }
    } 
	else LEDCounter = 0;
    
	// if(Count0 % 2 == 0) Key_Loop();
	// if(Count0 % 2 == 0) KeyProc();
	if(Count0 % 2 == 0) MatrixKey_Loop();
	if(Count0 % 2 == 0) KeyProc();

	if(NixieCount % 2 == 0) Nixie_Loop();
	if(NixieCount % 2 == 0) {
		ADValue = XPT2046_ReadAD(XPT2046_XP);
		if(ADValue < 8) ADValue = 8;
		else if(ADValue > 195) ADValue = 196;
		AD_Set = ADValue;
	}
    if(Count0 > 4000 && mode == 0) { Count0 = 0; mode = 1; }
    
    if(Count0 % 1000 == 0 && mode == 2) {
        LEDFlag = ~LEDFlag;
        if(LEDFlag) P2 = 0xff;
        else P2 = 0x00;
    }
}