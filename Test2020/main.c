#include <REGX52.H>
#include "Delay.h"
#include "Key.h"
#include "Nixie.h"
#include "Motor.h"
#include "IR.h"
#include "LCD1602.h"
#include "Buzzer.h"
#include <STDIO.H>
#include "DS1302.h"
#include "string.h"
#include "DS18B20.h"
#include "AT24C02.h"
#include "Timer0.h"
#include "Timer1.h"

/*PD*/
#define LINE_MAX_LENGTH 20
/*PD End*/
/*PV*/

unsigned char Time_5s_Count = 0;    // 用于5秒温度任务
unsigned char Time_60s_Count = 0;   // 用于60秒掉电存储（可选）

bit Flag_1s_Task = 0;       // 1秒任务（DS1302读取、LCD刷新）
bit Flag_5s_Task = 0;       // 5秒任务（DS18B20开始转换）
bit Flag_750ms_Ready = 0;   // DS18B20 750ms 转换完成标志
bit Flag_60s_Task = 0;      // 60秒任务（AT24C02备份）

// DS18B20 状态机
char Temp_State = 0; // 0: Idle, 1: Convert Started, 2: Read Value

typedef enum {
	Calendar,
	Safe,
	Calculator
} Mode;
Mode mode=Calendar;



unsigned char IR_Cmd=0;

xdata char Init_Time[]={19,12,31,23,59,0,2};	//初始时间
// char Time[7];
xdata char L1_String[LINE_MAX_LENGTH]="";
xdata char L2_String[LINE_MAX_LENGTH]="";

/*PV End*/

/*Module Code*/
void IR_Data_Proc(void)
{
	if (IR_GetDataFlag()){IR_Cmd=IR_GetCommand();}
}
void Mode_Change(void){
	if (IR_Cmd == IR_MODE){
		mode = (Mode)((mode + 1) % 3);
		IR_Cmd = 0;
	}else if (IR_Cmd == IR_POWER){
		mode = Calendar;	//重置
		IR_Cmd = 0;
	}
}
void Calendar_Init(void){
	//DS18B20 因为初始化需要一定时间，前置到主函数循环第一项
	DS1302_Init();
	memcpy(DS1302_Time,Init_Time,sizeof(Init_Time));
	DS1302_SetTime();
}
void Calendar_Disp(void){

	sprintf(L1_String,"20%02d-%02d-%02d",DS1302_Time[0],DS1302_Time[1],DS1302_Time[2]);
	sprintf(L2_String,"%02d:%02d:%02d",DS1302_Time[3],DS1302_Time[4],DS1302_Time[5]);
	LCD_ShowString(1,1,L1_String);
	LCD_ShowString(2,1,L2_String);
}

void Calendar_Set(void){
	
}


/*Module Code End*/

/*Proc Code*/
// DS18B20 状态机处理函数 (非阻塞)
void Temp_Proc(void){
    if (mode != Calendar) return;

    if (Temp_State == 0) { // Idle 状态
        if (Flag_5s_Task) {
            // 开始温度转换，不阻塞
            DS18B20_ConvertT(); // 假设这个函数只是发送启动命令
            Flag_5s_Task = 0;
            Temp_State = 1; // 进入转换中状态
        }
    } else if (Temp_State == 1) { // 转换中状态
        if (Flag_750ms_Ready) {
            // 750ms 等待完成，读取温度
            DS18B20_ReadT(); // 假设你的 DS18B20.c 中有一个读取函数
            
            // TODO: 在这里格式化并显示温度值
            // sprintf(L2_String + 9, " T:%2.2f", DS18B20_Temperature);
            // LCD_ShowString(2, 1, L2_String);
            
            Flag_750ms_Ready = 0;
            Temp_State = 0; // 回到 Idle 状态
        }
    }
}

void Calendar_Proc(void){
	bit Cal_Mode = 0; //0:Display, 1:Set
	if (mode != Calendar) {return;}
    
	// 1. 模式切换处理 (保持不变)
	if (IR_Cmd == IR_START_STOP){
		Cal_Mode = !Cal_Mode;
		IR_Cmd = 0;
        // 如果进入设置模式，可以清除 LCD
	}
    
	// 2. 周期性任务处理
    if (Cal_Mode) {
		Calendar_Set();
	}else {
        // 1秒任务：时间读取和显示
        if (Flag_1s_Task) {
            DS1302_ReadTime(); // 从DS1302读取时间
            Calendar_Disp();
            Flag_1s_Task = 0;
        }
        
        // 5秒任务：温度转换和读取 (通过 Temp_Proc 状态机处理)
        Temp_Proc();
        
        // 60秒任务：掉电存储备份
        if (Flag_60s_Task) {
            // AT24C02_WriteTime(); // 将时间写入EEPROM
            Flag_60s_Task = 0;
        }
	}
}

void Safe_Proc(void){
	if (mode != Safe) {return;} 
}

void Calculator_Proc(void){
	if (mode != Calculator) {return;}
}
/*Proc Code End*/

void main()
{
	// DS18B20_ConvertT();
	// Delay(750);
	LCD_Init();
	Calendar_Init();
	IR_Init();
	Motor_Init();
	while(1)
	{
		IR_Data_Proc();
		Mode_Change();

		Calendar_Proc();
		Safe_Proc();
		Calculator_Proc();
	}
}

