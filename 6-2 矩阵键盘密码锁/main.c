#include <REGX52.H>
#include "LCD1602.h"
#include "delay.h"
#include "MatrixKey.h"

unsigned char KeyNum;
unsigned int Password,Count;

void main()
{
    LCD_Init();
    LCD_ShowString(1,1,"Password:");
    while(1)
    {
        KeyNum = MatrixKey();
        if(KeyNum)
        {
            if(KeyNum <= 10)
            {
                if(Count < 4)
                {
                    Password *= 10;
                    Password += KeyNum % 10;
                    Count++;
                }
                LCD_ShowNum(2,1,Password,4);
            }
            if(KeyNum == 11)
            {
                if(Password == 1234)
                {
                    LCD_ShowString(1,14,"OK ");
                    Password = 0;
                    Count = 0;
                    LCD_ShowNum(2,1,Password,4);
                    LCD_ShowString(2,1,"    ");
                }
                else
                {
                    LCD_ShowString(1,14,"ERR");
                    Password = 0;
                    Count = 0;
                    LCD_ShowNum(2,1,Password,4);
                    LCD_ShowString(2,1,"    ");
                }
            }
            if(KeyNum == 12)
            {
                Password = 0;
                Count = 0;
                LCD_ShowNum(2,1,Password,4); 
                LCD_ShowString(2,1,"    ");
                LCD_ShowString(1,14,"   ");
            }
        }
    }
}