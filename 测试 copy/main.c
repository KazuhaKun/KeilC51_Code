#include <REGX52.H>

//数码管
unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,0x00};
//LED
unsigned char LEDCode[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

//LED位置检测
unsigned char detectLEDPosition()
{
    unsigned char position;
    for (position = 0; position < 7; position++)
    {
        if (P2 == LEDCode[position])
        {
            return position + 1; 
        }
    }
    return 0;
}

//数码管显示函数
void Nixie(unsigned char Location, unsigned char Num)
{
    P2_2 = Location & 0x01;
    P2_3 = (Location >> 1) & 0x01;
    P2_4 = (Location >> 2) & 0x01;
    P0 = NixieCode[Num];
    Delay(1);
    P0 = 0x00;
}

//一些变量
unsigned int t, i;
unsigned int b = 500;
unsigned char led_num = 0;

//K2 延时增加函数
void delayk2()
{
    if (P3_0 == 0) {
		Delay(20);
        while(P3_0 == 0);
        Delay(20);
        b += 500;
		if (b > 2000) b = 500;
	}
}

//模式一
void mode1(unsigned char Location1)
{
    for (t = 0; t < b; t++) {
        P2 = LEDCode[Location1];
        Nixie(7,Location1+1);
        if (P3_0 == 0) break;
        if (P3_2 == 0) break;
        if (P3_3 == 0) break;
    }
    delayk2();
}		

void main()
{
    unsigned char led_status = 0;
    unsigned int mode = 0;
//开机显示学号后四位
    for (i = 0; i < 1000; i++)
    {
        Nixie(5,1);
        Nixie(4,9);
        Nixie(3,2);
        Nixie(2,0);
        if (P3_1 == 0 || P3_3 == 0) break;
        if (P3_2 == 0)
        {
            P2 = ~led_status;
            break;
        }
    }
//主功能
    while(1)
    {
        if (P3_1 == 0) mode = 1;
        if (P3_2 == 0) mode = 2;
        if (P3_3 == 0) {
            mode = 3;
            led_num = 0;
        }

        switch (mode) 
        {
//主功能一
            case 1: 
            {
                mode1(led_num);
                if (led_num < 8) led_num++;
                if (led_num == 8) led_num = 0;
                break;
            }
//主功能二
            case 2: 
            {
                if(P3_2 == 0)
                {
                    Delay(100);
                    while(P3_2 == 0);
                    Delay(100);
                    led_status <<= 1;
                    
                    if (led_status == 0) led_status = 0x01;
                    
                    while (P3_2 == 1) 
                    {
                        P2 = ~led_status;
                        if (P2 != 0x7F) Nixie(7, detectLEDPosition());

                        if (P2 == 0x7F) for (i = 0; i < 8; i++) for (t = 0; t < 500; t++) 
                        {
                            Nixie(7, i+1);
                            if (P3_1 == 0 || P3_2 == 0 || P3_3 == 0) break;
                        }
                        if (P3_1 == 0 || P3_2 == 0 || P3_3 == 0) break;
                    }
                }
                break;
            }
//主功能三
            case 3: 
            {
                Delay(20);
                while(P3_3 == 0);
                Delay(20);

                for (i = 0; i < 8; i++) for (t = 0; t < 500; t++) 
                {
                    Nixie(7-i, 8);
                    P2 = 0x00;
                    if (P3_1 == 0 || P3_2 == 0) break;
                }
                break;
            }
        }
    }
}