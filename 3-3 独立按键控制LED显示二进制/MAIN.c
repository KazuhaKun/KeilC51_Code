#include <REGX52.H>
//#include <INTRINS.H>

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

/*unsigned char reverseBits(unsigned char num) {
    num = ((num & 0xF0) >> 4) | ((num & 0x0F) << 4);
    num = ((num & 0xCC) >> 2) | ((num & 0x33) << 2);
    num = ((num & 0xAA) >> 1) | ((num & 0x55) << 1);
    return num;
}
*/

void main()
{
	unsigned char led_status = 0x00;
	P2 = ~led_status;
//	P2 = reverseBits(~led_status);
	while(1)
	{
		if (P3_1==0)
		{
			Delay(20);
			while(P3_1==0);
			Delay(20);
			led_status++;
//			P2 = reverseBits(~led_status);
			P2 = ~led_status;
		}
	}
}
