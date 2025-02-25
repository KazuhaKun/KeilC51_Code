#include <REGX52.H>
#include "Timer0.h"
#include "Delay.h"

unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x40, 0x00};
unsigned char LEDCode[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

unsigned char led_num = 0;
unsigned char nixie_digits[] = {1, 8, 8, 6};

unsigned int led_delay_count = 0;
unsigned char delay_count;
unsigned char delay_flag;
unsigned int total_time_count = 0;

unsigned int T0Count;


unsigned char hour=1;
unsigned char min=15;
unsigned char sec=50;

unsigned int display_time = 0; 
unsigned int delay1_count = 0; 
unsigned int delay2_count = 0; 
unsigned char timer2_active = 1; 
unsigned int led_blink_count = 0; 
unsigned char mode0_done = 0; 
unsigned char led_blink_500ms = 0; 
unsigned char display_off = 0;
unsigned char delay_count=0;
unsigned char mode1_done = 0;  
unsigned char sec_double_speed = 0; 
unsigned int sec_double_speed_count = 0;
void Nixie(unsigned char Location, unsigned char Num)
{
    P2_2 = Location & 0x01;
    P2_3 = (Location >> 1) & 0x01;
    P2_4 = (Location >> 2) & 0x01;
    P0 = NixieCode[Num];
    Delay(1);
    P0 = 0x00;
}

void led()
{
	Nixie(7,hour/10);
	Nixie(6,hour%10);
	Nixie(4,min/10);
	Nixie(3,min%10);
	Nixie(1,sec/10);
	Nixie(0,sec%10);
}
unsigned int mode =0;
void main()
{
    Timer0Init();  
		
    while(1)
    {
        if (!mode0_done) 
        {
            mode = 0;
        }
        
        if (hour == 1 && min == 16 && sec == 0) 
        {
            display_off = 1; 
            sec_double_speed = 1; 
        }
        if (hour == 1 && min == 16 && sec == 30) 
        {
            display_off = 0; 
            mode = 2; 
        }
				if(P3_1==0)break;
        switch(mode)
        {
            case 0:
            {
                Nixie(5,1);
                Nixie(4,8);
                Nixie(3,8);
                Nixie(2,6);
                P2 = LEDCode[led_num]; 
                break; 
            }
            case 1:
            {
                if (display_off == 1)
                {
                    P0 = 0x00; 
                    P2 = LEDCode[led_num]; 
                }
                else
                {
                    Nixie(5,10);
                    Nixie(2,10);
                    led();
                }
                mode1_done = 1; 
                break; 
            }
            case 2:
            {
                Nixie(7, hour / 10);
                Nixie(6, hour % 10);
                Nixie(4, min / 10);
                Nixie(3, min % 10);
                Nixie(1, sec / 10);
                Nixie(0, sec % 10);
								Nixie(5,10);
                    Nixie(2,10);
                break; 
            }
        }
    }
}
			
		
void Timer0_Routine(void) interrupt 1
{
    TL0 = 0x66;		
    TH0 = 0xFC;
    
    if (timer2_active)
    {
        delay2_count++;
			delay_count++;
        if (delay2_count >= 4000) 
        {
            delay2_count = 0;
            timer2_active = 0; 
            mode0_done = 1; 
        }
        if (delay2_count % 400 == 0) 
        {
            led_num++;
            if (led_num >= 8) led_num = 0;
            P2 = LEDCode[led_num]; 
        }
    }
    else
    {
			if (display_off != 10)
			{
        delay1_count++;
        if (delay1_count >= 1000) 
        {
            delay1_count = 0;
            mode = 1;
            sec++;
            if(sec >= 60)
            {
                sec = 0;
                min++;
                if(min >= 60)
                {
                    min = 0;
                    hour++;
                    if(hour >= 24) hour = 0;
                }
            }
        }
			}
        if (display_off == 1 && mode == 1)
        {
            sec_double_speed_count++;
            if (sec_double_speed_count % 500 == 0) 
            {
                led_num++;
                if (led_num >= 8) led_num = 0;
                P2 = LEDCode[led_num]; 
            }

            
            if (sec_double_speed_count >=2000) 
            {
                sec_double_speed_count = 0;
                sec++;
                if(sec >= 60) 
                {
                    sec = 0;
                    min++;
                    if(min >= 60)
                    {
                        min = 0;
                        hour++;
                        if(hour >= 24) hour = 0;
                    }
                }
            }
        }
    }
}
