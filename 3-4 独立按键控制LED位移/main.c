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


void main()
{
	unsigned char led_status = 0x01;
	P2 = ~led_status;
	while(1)
	{
		if (P3_1==0)
		{
			Delay(20);
			while(P3_1==0);
			Delay(20);
			led_status <<= 1;
            if (led_status == 0)
            {
                led_status = 0x01;
            }
			P2 = ~led_status;
		}
	}
}
