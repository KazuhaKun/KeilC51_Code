#include <REGX52.H>
// #include "delay.h"       // 若单片机速度过快，可加入延时

sbit DS1302_SCLK = P3^6;
sbit DS1302_IO = P3^4;
sbit DS1302_CE = P3^5;

unsigned char DS1302_Time_DEC[7] = {0,0,0,1,15,30,0}; //DEC
unsigned char DS1302_Time_BCD[7] = {0,0,0,0,0,0,0}; //BCD
unsigned char DS1302_RegRead[7] = {0x8D,0x89,0x87,0x85,0x83,0x81,0x8B};
unsigned char DS1302_RegWrite[7] = {0x8C,0x88,0x86,0x84,0x82,0x80,0x8A};

void DS1302_Init(void)
{
    DS1302_CE = 0;
    DS1302_SCLK = 0;
}

void DS1302_WriteByte(unsigned char Command,Data)
{
    unsigned char i;
    DS1302_CE = 1;
    for(i = 0; i < 8; i++)
    {
        DS1302_IO = Command & (0x01 << i);
        DS1302_SCLK = 1;
        // Delay(1);    // 若单片机速度过快，可加入延时
        DS1302_SCLK = 0;
    }
    for(i = 0; i < 8; i++)
    {
        DS1302_IO = Data & (0x01 << i);
        DS1302_SCLK = 1;
        // Delay(1);    // 若单片机速度过快，可加入延时
        DS1302_SCLK = 0;
    }
    DS1302_CE = 0;
}

unsigned char DS1302_ReadByte(unsigned char Command)
{
    unsigned char i, Data = 0x00;
    DS1302_CE = 1;
    for(i = 0; i < 8; i++)
    {
        DS1302_IO = Command & (0x01 << i);
        DS1302_SCLK = 0;
        // Delay(1);    // 若单片机速度过快，可加入延时
        DS1302_SCLK = 1;
    }
    for(i = 0; i < 8; i++)
    {
        DS1302_SCLK = 1;
        // Delay(1);    // 若单片机速度过快，可加入延时
        DS1302_SCLK = 0;
        if(DS1302_IO){Data |= (0x01 << i);}
    }
    DS1302_CE = 0;
    DS1302_IO = 0;
    return Data;
}

//8C 88 86 84 82 80 8A
void DS1302_SetTime(void)
{
    unsigned char i;
    DS1302_WriteByte(0x8E, 0x00);
    for(i = 0; i < 7; i++)
    {
        DS1302_Time_BCD[i] = DS1302_Time_DEC[i] / 10 * 16 + DS1302_Time_DEC[i] % 10;    //DEC TO BCD
        DS1302_WriteByte(DS1302_RegWrite[i], DS1302_Time_BCD[i]);
    }
}

void DS1302_ReadTime(void)
{
    unsigned char i;
    for(i = 0; i < 7; i++)
    {
        DS1302_Time_BCD[i] = DS1302_ReadByte(DS1302_RegRead[i]);
        DS1302_Time_DEC[i] = DS1302_Time_BCD[i] / 16 * 10 + DS1302_Time_BCD[i] % 16;    //BCD TO DEC
    }
}