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
	while(1)
	{
//		P2_0=0;
		if (P3_1==0)
		{
			Delay(20);
			while(P3_1==0);
			Delay(20);
			
			P2_0=~P2_0;
		}
	}
}

/*void Delay(unsigned int xms)		//@11.0592MHz
{
	unsigned char i, j;
	while(xms)
	{
		_nop_();
		_nop_();
		_nop_();
		i = 11;
		j = 190;
		do
		{
		while (--j);
		} while (--i);
		
		xms--;
	}

}
*/
/*#include <REGX52.H>

void main()
{
//	P2=0xFE;
	while(1)
	{
		if(P3_1==0)
		{
			P2_0=0;
		}
		else
		{
			P2_0=1;
		}
	}
}
*/