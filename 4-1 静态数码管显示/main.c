#include <REGX52.H>

unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void Delay(unsigned int xms)		//@11.0592MHz
{
	unsigned char i, j;
	while(xms)
	{
//	_nop_();
		i = 2;
		j = 199;
		do
		{
			while (--j);
			} while (--i);
		xms--;

	}
}

void Nixie(unsigned char Location, unsigned char Num)
{
    P2_2=Location&0x01;
    P2_3=(Location>>1)&0x01;
    P2_4=(Location>>2)&0x01;
    P0=NixieCode[Num];
		Delay(1);
		P0=0x00;
}

void main()
{

    while(1)
    {
			Nixie(7,1);
//			Delay(1);
			Nixie(6,2);
//			Delay(1);
			Nixie(5,3);
//			Delay(1);
    }
}