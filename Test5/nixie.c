#include <REGX52.H>
#include "delay.h"

unsigned char Nixie_Buf[9] = {0,10,10,10,10,10,10,10,10};

unsigned char Nixie_Point[9] = {0,0,0,0,0,0,0,0,0};

unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,0x00,0x40};

void Nixie_SetBuf(unsigned char Location,unsigned char Num,unsigned char Point)
{
    Nixie_Buf[Location] = Num;
    if(Point) Nixie_Point[Location] = 1;
    else Nixie_Point[Location] = 0;
}

void Nixie_Scan(unsigned char Location,unsigned char Num,unsigned char Point)
{
//MyWay
    /*
    P0 = 0x00;

    Location = 8 - Location;
    P2_2 = Location & 0x01;
    P2_3 = (Location >> 1) & 0x01;
    P2_4 = (Location >> 2) & 0x01;

    P0 = NixieCode[Num];
*/

    P0=0x00;				//段码清0，消影
	switch(Location)		//位码输出
	{
		case 1:P2_4=1;P2_3=1;P2_2=1;break;
		case 2:P2_4=1;P2_3=1;P2_2=0;break;
		case 3:P2_4=1;P2_3=0;P2_2=1;break;
		case 4:P2_4=1;P2_3=0;P2_2=0;break;
		case 5:P2_4=0;P2_3=1;P2_2=1;break;
		case 6:P2_4=0;P2_3=1;P2_2=0;break;
		case 7:P2_4=0;P2_3=0;P2_2=1;break;
		case 8:P2_4=0;P2_3=0;P2_2=0;break;
	}
    if(Point){P0=NixieCode[Num]|0x80;}	//带小数点
    else P0=NixieCode[Num];	//段码输出
}

void Nixie_Loop(void)
{
    static unsigned char i;

    Nixie_Scan(i,Nixie_Buf[i],Nixie_Point[i]);
    i++;
    if(i >= 9) i = 1;
}