#include <REGX52.H>

void Delay500ms()		//@11.0592MHz
{
	unsigned char i, j, k;

	;
	i = 4;
	j = 129;
	k = 119;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void main()
{
	while(1)
	{
		P2=0xFE; 
		Delay500ms();
		P2=0xFD; 
		Delay500ms();
		P2=0xFB;
		Delay500ms();
		P2=0xF7;
		Delay500ms();
		P2=0xEF;
		Delay500ms();
		P2=0xDF;
		Delay500ms();
		P2=0xBF;
		Delay500ms();
		P2=0x7F;
		Delay500ms();
	}
}