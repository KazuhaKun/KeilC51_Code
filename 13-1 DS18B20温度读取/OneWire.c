#include <REGX52.H>

sbit OneWire_DQ = P3^7;

unsigned char OneWire_Init(void)
{
	unsigned char i;
    unsigned char AckBit;
    OneWire_DQ = 1;
    OneWire_DQ = 0;
	i = 227;while (--i); //Delay 500us
    OneWire_DQ = 1;
	i = 29;while (--i); //Delay 70us
    AckBit = OneWire_DQ;
    i = 227;while (--i); //Delay 500us
    return AckBit;
}
void OneWire_SendBit(unsigned char Bit)
{
    unsigned char i;
    OneWire_DQ = 0;
	i = 4;while (--i); //Delay 10us
    OneWire_DQ = Bit;
    i = 23;while (--i); //Delay 50us
    OneWire_DQ = 1;
}

unsigned char OneWire_ReceiveBit(void)
{
    unsigned char i;
    unsigned char Bit;
    // OneWire_DQ = 1;
    OneWire_DQ = 0;
	i = 2;while (--i); //Delay 5us
    OneWire_DQ = 1;
	i = 2;while (--i); //Delay 5us
    Bit = OneWire_DQ;
    i = 23;while (--i); //Delay 50us
    return Bit;
}

void OneWire_SendByte(unsigned char Byte)
{
    unsigned char i;
    for(i=0;i<8;i++)
    {
        OneWire_SendBit(Byte & (0x01 << i));
    }
}

unsigned char OneWire_ReceiveByte(void)
{
    unsigned char i;
    unsigned char Byte=0x00;
    for(i=0;i<8;i++)
    {
        Byte |= OneWire_ReceiveBit() << i;    //前函数未上拉
        // if(OneWire_ReceiveBit()) Byte |= (0x01<<i);
    }
    return Byte;
}