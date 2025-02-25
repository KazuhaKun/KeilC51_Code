#include <REGX52.H>
#include "LCD1602.h"
#include "DS1302.h"
#include "delay.h"
#include "Key.h"
#include "Timer0.h"

unsigned char Year, Month, Date, Hour, Min, Sec, Day;
unsigned char KeyNum, TimeSetFlashFlag;

void LCD_ShowTime()
{		
		//8C 88 86 84 82 80 8A
		//8D 89 87 85 83 81 8B
		// Year = DS1302_ReadByte(0x8D);
		Year = DS1302_Time_DEC[0];
		LCD_ShowNum(1,7,Year,2);
		Month = DS1302_Time_DEC[1];
		LCD_ShowNum(1,10,Month,2);
		Date = DS1302_Time_DEC[2];
		LCD_ShowNum(1,13,Date,2);
		Hour = DS1302_Time_DEC[3];
		LCD_ShowNum(2,1,Hour,2);	
		Min = DS1302_Time_DEC[4];
		LCD_ShowNum(2,4,Min,2);	
		Sec = DS1302_Time_DEC[5];
		LCD_ShowNum(2,7,Sec,2);
		Day = DS1302_Time_DEC[6];
		LCD_ShowNum(1,16,Day,1);
		// Delay(50);
}

void main()
{
	//初始化LCD、DS1302
	LCD_Init();
	DS1302_Init();
	Timer0Init();
	//显示不变要素
	LCD_ShowString(1, 1, "RTC");
	LCD_ShowString(1,9,"/  /   ");
	LCD_ShowString(2, 3, ":  :");
	DS1302_SetTime();
	DS1302_ReadTime();
	while(1)
	{
		unsigned char i;
		unsigned char DS1302_Time_Backup[7];
		//模式选择
		unsigned char Mode;
		KeyNum = Key();
		if(KeyNum == 1)	Mode ++%= 9;
		switch(Mode)
		{
			case 0:
			{
				LCD_ShowString(2,12,"     ");
				DS1302_ReadTime();
				LCD_ShowTime();
				break;
			}
			case 1:
			{
				for(i = 0; i < 7; i++)
				{
					DS1302_Time_Backup[i] = DS1302_Time_DEC[i];
				}
				LCD_ShowString(2,12,"Year ");
				if(KeyNum == 2) DS1302_Time_DEC[0]++;
				if(KeyNum == 3 && DS1302_Time_DEC[0] != 0) DS1302_Time_DEC[0]--;
				else if(KeyNum == 3 && DS1302_Time_DEC[0] == 0) DS1302_Time_DEC[0] = 99;
				if(DS1302_Time_DEC[0] > 99) DS1302_Time_DEC[0] = 0;
				// if(DS1302_Time_DEC[0] < 0) DS1302_Time_DEC[0] = 99;
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				if(TimeSetFlashFlag == 1){LCD_ShowString(1,7,"  ");}
				else {LCD_ShowNum(1,7,DS1302_Time_DEC[0],2);}
				break;
			}
			case 2:
			{
				LCD_ShowNum(1,7,DS1302_Time_DEC[0],2);
				LCD_ShowString(2,12,"Month");
				if(KeyNum == 2) DS1302_Time_DEC[1]++;
				if(KeyNum == 3) DS1302_Time_DEC[1]--;
				if(DS1302_Time_DEC[1] > 12) DS1302_Time_DEC[1] = 1;
				if(DS1302_Time_DEC[1] < 1) DS1302_Time_DEC[1] = 12;
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				if(TimeSetFlashFlag == 1){LCD_ShowString(1,10,"  ");}
				else {LCD_ShowNum(1,10,DS1302_Time_DEC[1],2);}
				break;
			}
			case 3:
			{
				LCD_ShowNum(1,10,DS1302_Time_DEC[1],2);
				LCD_ShowString(2,12,"Date ");
				if(KeyNum == 2) DS1302_Time_DEC[2]++;
				if(KeyNum == 3) DS1302_Time_DEC[2]--;
				if(DS1302_Time_DEC[1] == 2)
				{
					if(DS1302_Time_DEC[0] % 4 == 0)
					{
						if(DS1302_Time_DEC[2] > 29) DS1302_Time_DEC[2] = 1;
						if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 29;
					}
					else
					{
						if(DS1302_Time_DEC[2] > 28) DS1302_Time_DEC[2] = 1;
						if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 28;
					}
				}
				else if(DS1302_Time_DEC[1] == 4 || DS1302_Time_DEC[1] == 6 || DS1302_Time_DEC[1] == 9 || DS1302_Time_DEC[1] == 11)
				{
					if(DS1302_Time_DEC[2] > 30) DS1302_Time_DEC[2] = 1;
					if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 30;
				}
				else
				{
					if(DS1302_Time_DEC[2] > 31) DS1302_Time_DEC[2] = 1;
					if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 31;
				}
				if(DS1302_Time_DEC[2] > 31) DS1302_Time_DEC[2] = 1;
				if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 31;     
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				if(TimeSetFlashFlag==1){LCD_ShowString(1,13,"  ");}
				else {LCD_ShowNum(1,13,DS1302_Time_DEC[2],2);}
				break;
			}
			case 4:
			{
				LCD_ShowNum(1,13,DS1302_Time_DEC[2],2);
				LCD_ShowString(2,12,"Day  ");
				if(KeyNum == 2) DS1302_Time_DEC[6]++;
				if(KeyNum == 3) DS1302_Time_DEC[6]--;
				if(DS1302_Time_DEC[6] > 7) DS1302_Time_DEC[6] = 1;
				if(DS1302_Time_DEC[6] < 1) DS1302_Time_DEC[6] = 7;
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				LCD_ShowNum(1,16,DS1302_Time_DEC[6],1);
				if(TimeSetFlashFlag == 1){LCD_ShowString(1,16,"  ");}
				else {LCD_ShowNum(1,16,DS1302_Time_DEC[6],1);}
				break;
			}
			case 5:
			{
				LCD_ShowNum(1,16,DS1302_Time_DEC[6],1);
				LCD_ShowString(2,12,"Hour ");
				if(KeyNum == 2) DS1302_Time_DEC[3]++;
				if(KeyNum == 3 && DS1302_Time_DEC[3] != 0) DS1302_Time_DEC[3]--;
				else if(KeyNum == 3 && DS1302_Time_DEC[3] == 0) DS1302_Time_DEC[3] = 23;
				if(DS1302_Time_DEC[3] > 23) DS1302_Time_DEC[3] = 0;
				// if(DS1302_Time_DEC[3] < 0) DS1302_Time_DEC[3] = 23;
				// if(DS1302_Time_DEC[3] == 0 && KeyNum == 3) DS1302_Time_DEC[3] = 23;
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				if(TimeSetFlashFlag == 1){LCD_ShowString(2,1,"  ");}
				else {LCD_ShowNum(2,1,DS1302_Time_DEC[3],2);}
				break;
			}
			case 6:
			{
				LCD_ShowNum(2,1,DS1302_Time_DEC[3],2);
				LCD_ShowString(2,12,"Min  ");
				if(KeyNum == 2) DS1302_Time_DEC[4]++;
				if(KeyNum == 3 && DS1302_Time_DEC[4] != 0) DS1302_Time_DEC[4]--;
				else if(KeyNum == 3 && DS1302_Time_DEC[4] == 0) DS1302_Time_DEC[4] = 59;
				if(DS1302_Time_DEC[4] > 59) DS1302_Time_DEC[4] = 0;
				// if(DS1302_Time_DEC[4] < 0) DS1302_Time_DEC[4] = 59;
				// if(DS1302_Time_DEC[4] == 0 && KeyNum == 3) DS1302_Time_DEC[4] = 59;
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				if(TimeSetFlashFlag == 1){LCD_ShowString(2,4,"  ");}
				else {LCD_ShowNum(2,4,DS1302_Time_DEC[4],2);}
				break;
			}
			case 7:
			{
				LCD_ShowNum(2,4,DS1302_Time_DEC[4],2);
				LCD_ShowString(2,12,"Sec  ");
				if(KeyNum == 2) DS1302_Time_DEC[5]++;
				if(KeyNum == 3 && DS1302_Time_DEC[5] !=0) DS1302_Time_DEC[5]--;
				else if(KeyNum == 3 && DS1302_Time_DEC[5] == 0) DS1302_Time_DEC[5] = 59;
				if(DS1302_Time_DEC[5] > 59) DS1302_Time_DEC[5] = 0;
				// if(DS1302_Time_DEC[5] < 0) DS1302_Time_DEC[5] = 59;
				// if(DS1302_Time_DEC[5] == 0 && KeyNum == 3) DS1302_Time_DEC[5] = 59;
				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
				if(TimeSetFlashFlag == 1){LCD_ShowString(2,7,"  ");}
				else {LCD_ShowNum(2,7,DS1302_Time_DEC[5],2);}
				break;
			}
			case 8:
			{
				for(i = 0; i < 7; i++)
				{
					DS1302_Time_DEC[i] = DS1302_Time_Backup[i];
				}
				DS1302_SetTime();
				LCD_ShowString(1,7,"  ");
				LCD_ShowNum(2,7,DS1302_Time_DEC[5],2);
				LCD_ShowString(2,12,"     ");
				
				Mode = 0;
				break;
			}
		}
	}
}

void Timer0_Routine() interrupt 1
{
	static unsigned int T0Count;
	TL0 = 0x66;		//设置定时初值
	TH0 = 0xFC;		//设置定时初值
	T0Count++;
	if(T0Count>=500)//每500ms进入一次
	{
		T0Count=0;
		TimeSetFlashFlag=!TimeSetFlashFlag;//闪烁标志位取反
	}
}

//K1 切换模式 
//M0:显示时间 M1:设置年份 M2:设置月份 M3:设置日期 M4:设置星期 M5:设置时钟 M6:设置分钟 M7:设置秒钟 M8:取消设置
//K2 增加 K3 减少
//K4 保存设置

// //加减分开 判断分开

// void DS1302_Time_Check(unsigned char TimeSelect)
// {
// 	switch(TimeSelect)
// 	// if(DS1302_Time_DEC[0] > 99) DS1302_Time_DEC[0] = 0;
// 	// if(DS1302_Time_DEC[1] > 12) DS1302_Time_DEC[1] = 1;
// 	// if(DS1302_Time_DEC[2] > 31) DS1302_Time_DEC[2] = 1;
// 	// if(DS1302_Time_DEC[3] > 23) DS1302_Time_DEC[3] = 0;
// 	// if(DS1302_Time_DEC[4] > 59) DS1302_Time_DEC[4] = 0;
// 	// if(DS1302_Time_DEC[5] > 59) DS1302_Time_DEC[5] = 0;
// 	// if(DS1302_Time_DEC[6] > 7) DS1302_Time_DEC[6] = 1;

// 	// if(DS1302_Time_DEC[1] == 2)
// 	// {
// 	// 	if(DS1302_Time_DEC[0] % 4 == 0)
// 	// 	{
// 	// 		if(DS1302_Time_DEC[2] > 29) DS1302_Time_DEC[2] = 1;
// 	// 		if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 29;
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		if(DS1302_Time_DEC[2] > 28) DS1302_Time_DEC[2] = 1;
// 	// 		if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 28;
// 	// 	}
// 	// }
// 	// else if(DS1302_Time_DEC[1] == 4 || DS1302_Time_DEC[1] == 6 || DS1302_Time_DEC[1] == 9 || DS1302_Time_DEC[1] == 11)
// 	// {
// 	// 	if(DS1302_Time_DEC[2] > 30) DS1302_Time_DEC[2] = 1;
// 	// 	if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 30;
// 	// }
// 	// else
// 	// {
// 	// 	if(DS1302_Time_DEC[2] > 31) DS1302_Time_DEC[2] = 1;
// 	// 	if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 31;
// 	// }
// }



// 			case TimeSelect:
// 			{
// 				if(KeyNum == 2) DS1302_Time_DEC[TimeSelect]++;
// 				if(KeyNum == 3 && DS1302_Time_DEC[TimeSelect] != 0) DS1302_Time_DEC[TimeSelect]--;

// 				else if(KeyNum == 3 && TimeSelect == 1 && DS1302_Time_DEC[TimeSelect] == 0) DS1302_Time_DEC[TimeSelect] = 99;
// 				else if(KeyNum == 3 && TimeSelect == 2 && DS1302_Time_DEC[TimeSelect] == 0) DS1302_Time_DEC[TimeSelect] = 12;

// 				else if(KeyNum == 3 && TimeSelect == 3 && DS1302_Time_DEC[TimeSelect] == 0) DS1302_Time_DEC[TimeSelect] = 31;

// 				else if(KeyNum == 3 && TimeSelect == 4 && DS1302_Time_DEC[TimeSelect] == 0) DS1302_Time_DEC[TimeSelect] = 7;
// 				else if(KeyNum == 3 && TimeSelect == 5 && DS1302_Time_DEC[TimeSelect] == 0) DS1302_Time_DEC[TimeSelect] = 23;
// 				else if(KeyNum == 3 && (TimeSelect == 6 |TimeSelect == 7) && DS1302_Time_DEC[TimeSelect] == 0) DS1302_Time_DEC[TimeSelect] = 59;
// 				if(DS1302_Time_DEC[0] > 99) DS1302_Time_DEC[0] = 0;


// 				if(KeyNum == 4) {DS1302_SetTime(); Mode = 0;}
// 				if(TimeSetFlashFlag == 1){LCD_ShowString(1,7,"  ");}
// 				else {LCD_ShowNum(1,7,DS1302_Time_DEC[0],2);}
// 				break;
// 			}

// 				if(DS1302_Time_DEC[1] == 2)
// 				{
// 					if(DS1302_Time_DEC[0] % 4 == 0)
// 					{
// 						if(DS1302_Time_DEC[2] > 29) DS1302_Time_DEC[2] = 1;
// 						if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 29;
// 					}
// 					else
// 					{
// 						if(DS1302_Time_DEC[2] > 28) DS1302_Time_DEC[2] = 1;
// 						if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 28;
// 					}
// 				}
// 				else if(DS1302_Time_DEC[1] == 4 || DS1302_Time_DEC[1] == 6 || DS1302_Time_DEC[1] == 9 || DS1302_Time_DEC[1] == 11)
// 				{
// 					if(DS1302_Time_DEC[2] > 30) DS1302_Time_DEC[2] = 1;
// 					if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 30;
// 				}
// 				else
// 				{
// 					if(DS1302_Time_DEC[2] > 31) DS1302_Time_DEC[2] = 1;
// 					if(DS1302_Time_DEC[2] < 1) DS1302_Time_DEC[2] = 31;
// 				}
