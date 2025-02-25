#include <REGX52.H>

//数码管
unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
//LED
unsigned char LEDCode[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

//延时函数
void Delay(unsigned int xms)		//@11.0592MHz
{
    unsigned char i, j;
    while(xms)
    {
        i = 2;
        j = 199;
        do
        {
            while (--j);
        } while (--i);
        xms--;
    }
}

//数码管显示函数
void Nixie(unsigned char Location0, unsigned char Num0)
{
    P2_2=Location0&0x01;
    P2_3=Location0&0x02;
    P2_4=Location0&0x03;
    P0=NixieCode[Num0];
    Delay(1);
    P0=0x00;
}

unsigned int t = 500;

//按键2延时函数
void delayk2()
{
    if (P3_0 == 0)
      {
					Delay(20);
          while(P3_0 == 0);
          Delay(20);
          t += 500; 
					if (t > 2000)
			{
					t = 500;
			}

			}
}

//模式一
void mode1(unsigned char Location1)
{
//    P2 = LEDCode[Location1];
    Nixie(7,LEDCode[Location1]);
//    Delay(t);
//    delayk2();
}							

void main()
{
        
    while(1)
    {
         Nixie(7,0);
    }
}