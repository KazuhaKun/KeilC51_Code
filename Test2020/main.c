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
// #include "string.h"
#include "DS18B20.h"
#include "AT24C02.h"
#include "Timer0.h"
#include "Timer1.h"

/*PD*/
#define LINE_MAX_LENGTH 16
#define CALENDAR_INPUT_LEN 14

/*PD End*/
/*PV*/

unsigned char Time_5s_Count = 0;    // 用于5秒温度任务

bit Flag_100ms_Task = 0;	// 100毫秒任务（DS1302读取、LCD刷新（新））
bit Flag_500ms_Task = 0;	// 500毫秒任务（设置闪烁）
bit Flag_1s_Task = 0;       // 1秒任务（DS1302读取、LCD刷新）
bit Flag_5s_Task = 0;       // 5秒任务（DS18B20开始转换）
bit Flag_750ms_Ready = 0;   // DS18B20 750ms 转换完成标志

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
xdata char Set_Time[7];
xdata char Plause_Time[7];
xdata char L1_String[LINE_MAX_LENGTH]="";
xdata char L2_String[LINE_MAX_LENGTH]="";

xdata int Temperature = 20; // 用于存储读取的温度值
xdata int Temperature_Point = 0; // 用于存储温度的小数部分

#define MAX_INPUT_DIGITS 16 // 最大支持16位数字输入

// 输入结构体，用于存储输入状态
typedef struct {
    unsigned char Digit_Buffer[MAX_INPUT_DIGITS]; // 存储输入的数字（0-9的数值）
    unsigned char Target_Length;                  // 目标输入长度
    unsigned char Current_Cursor;                 // 当前光标位置 (0 到 Target_Length - 1)
    unsigned char Input_Finished;                  // 输入完成标志 (通常由确认键设置)
    unsigned char Blinking_State;                  // 用于闪烁显示的标志
} IR_Input_State;

// 全局变量，用于保存当前输入状态
xdata IR_Input_State Current_Input;

/*PV End*/

/*Module Code*/
void AT24C02_clearTime(void){
	unsigned char i;
	for(i=0; i<7; i++){
		AT24C02_WriteByte(0x00 + i, 0x00); 	
		Delay(5);
	}
}
void LCD1602_Test(void){
	LCD_ShowString(1,1,"Hello,World!    ");
	LCD_ShowString(2,1,"LCD1602 Test    ");
}

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

// /**
//  * @brief 初始化红外输入模块 初始化长度、初始值、光标位置、输入状态等
//  * @param Target_Length 目标输入的数字位数 (例如：日历时间设置总共14位)
//  * @param Initial_Values 可选的初始值数组 (如果需要预填充)，可传入 NULL
//  */
// void IR_Input_Init(unsigned char Target_Length, unsigned char *Initial_Values)
// {
//     unsigned char i;
    
//     // 限制最大长度
//     if (Target_Length > MAX_INPUT_DIGITS) {
//         Target_Length = MAX_INPUT_DIGITS;
//     }

//     Current_Input.Target_Length = Target_Length;
//     Current_Input.Current_Cursor = 0; // 光标从第一位开始
//     Current_Input.Input_Finished = 0; // 输入未完成

//     // 清空或初始化缓冲区
//     for (i = 0; i < MAX_INPUT_DIGITS; i++)
//     {
//         if (i < Target_Length && Initial_Values != NULL) {
//             // 如果提供了初始值，则填充
//             Current_Input.Digit_Buffer[i] = Initial_Values[i];
//         } else {
//             // 否则初始化为 0 或其他占位符
//             Current_Input.Digit_Buffer[i] = 0; 
//         }
//     }
// }
// 将红外命令转换为对应的数字 (0-9)，如果不是数字则返回 0xFF
unsigned char IR_Cmd_To_Digit(unsigned char cmd) {
    switch (cmd) {
        case IR_0: return 0;
        case IR_1: return 1;
        case IR_2: return 2;
        case IR_3: return 3;
        case IR_4: return 4;
        case IR_5: return 5;
        case IR_6: return 6;
        case IR_7: return 7;
        case IR_8: return 8;
        case IR_9: return 9;
        default: return 0xFF; // 返回无效值
    }
}

void Calendar_Init(void){
    unsigned char i;
	DS1302_Init();
	// EEPROM 读时间
    for(i=0; i<7; i++){
        DS1302_Time[i] = AT24C02_ReadByte(0x00 + i);
    }
    
    // 检查数据有效性
    if (DS1302_Time[0] == 0x00) { 
        const char code Init_Time[] = {19,12,31,23,59,0,2}; 
        for(i=0; i<sizeof(Init_Time); i++){
            DS1302_Time[i] = Init_Time[i];
        }
    }
    // 设置时间到 DS1302
	DS1302_SetTime();
    
	// 备份时间到 EEPROM
    for(i=0; i<7; i++){
        AT24C02_WriteByte(0x00 + i, DS1302_Time[i]); 
        Delay(5);
    }
}

void Calendar_Disp(void){
	DS1302_ReadTime(); // 从DS1302读取时间


	// LCD_ShowNum(1,1,DS1302_Time[0],2);
	// LCD_ShowChar(1,3,'-');
	// LCD_ShowNum(1,4,DS1302_Time[1],2);
	// LCD_ShowChar(1,6,'-');
	// LCD_ShowNum(1,7,DS1302_Time[2],2);

	// LCD_ShowNum(2,1,DS1302_Time[3],2);
	// LCD_ShowChar(2,3,':');
	// LCD_ShowNum(2,4,DS1302_Time[4],2);
	// LCD_ShowChar(2,6,':');
	// LCD_ShowNum(2,7,DS1302_Time[5],2);

	// // 温度显示
	// LCD_ShowNum(2,11,Temperature,2);
	// LCD_ShowChar(2,13,'.');
	// LCD_ShowNum(2,14,Temperature_Point,2);
	// LCD_ShowChar(2,16,'C');

	sprintf(L1_String,"20%02bu-%02bu-%02bu      ",	//格式化字符串 占满16个字符
		DS1302_Time[0],
		DS1302_Time[1],
		DS1302_Time[2]
	);
	LCD_ShowString(1,1,L1_String);

	sprintf(L2_String,"%02bu:%02bu:%02bu  %d.%dC ",
        DS1302_Time[3],
        DS1302_Time[4],
        DS1302_Time[5],
        Temperature,
        Temperature_Point
	);
	LCD_ShowString(2,1,L2_String);
}

void Time_Backup(void){
	//每秒备份秒
	AT24C02_WriteByte(0x05, DS1302_Time[5]);
	// Delay(5);
	//每分钟备份分
	if (DS1302_Time[5] == 0) {
		AT24C02_WriteByte(0x04, DS1302_Time[4]);
		// Delay(5);
	}
	//每小时备份时
	if (DS1302_Time[4] == 0 && DS1302_Time[5] == 0) {
		AT24C02_WriteByte(0x03, DS1302_Time[3]);
		// Delay(5);
	}
	// //每天备份日、月、年、星期
	// if (DS1302_Time[3] == 0 && DS1302_Time[4] == 0 && DS1302_Time[5] == 0) {
	// 	AT24C02_WriteByte(0x02, DS1302_Time[2]);
	// 	Delay(5);
	// 	AT24C02_WriteByte(0x01, DS1302_Time[1]);
	// 	Delay(5);
	// 	AT24C02_WriteByte(0x00, DS1302_Time[0]);
	// 	Delay(5);
	// 	AT24C02_WriteByte(0x06, DS1302_Time[6]);
	// 	Delay(5);
	// }
}

/**
 * @brief 初始化红外输入模块 初始化长度、初始值、光标位置、输入状态等
 * @param Target_Length 目标输入的数字位数 (例如：日历时间设置总共14位)
 * @param Initial_Values 可选的初始值数组 (如果需要预填充)，可传入 NULL
 */
void IR_Input_Init(unsigned char Target_Length, unsigned char *Initial_Values)
{
    unsigned char i;
    
    // 限制最大长度
    if (Target_Length > MAX_INPUT_DIGITS) {
        Target_Length = MAX_INPUT_DIGITS;
    }

    Current_Input.Target_Length = Target_Length;
    Current_Input.Current_Cursor = 0; // 光标从第一位开始
    Current_Input.Input_Finished = 0; // 输入未完成

    // 清空或初始化缓冲区
    for (i = 0; i < MAX_INPUT_DIGITS; i++)
    {
        if (i < Target_Length && Initial_Values != NULL) {
            // 如果提供了初始值，则填充
            Current_Input.Digit_Buffer[i*2] = Initial_Values[i]/10;
			Current_Input.Digit_Buffer[i*2+1] = Initial_Values[i]%10;
        } else {
            // 否则初始化为 0 或其他占位符
            Current_Input.Digit_Buffer[i*2] = 0; 
			Current_Input.Digit_Buffer[i*2+1] = 0;
        }
    }
}

void IR_Input_Proc(unsigned char ir_digit){
	if(Current_Input.Target_Length == 0 || Current_Input.Input_Finished){
		return; // 如果没有设置目标长度或输入已完成，直接返回
	}
	Current_Input.Digit_Buffer[Current_Input.Current_Cursor] = ir_digit;

	Current_Input.Current_Cursor = (Current_Input.Current_Cursor + 1) % Current_Input.Target_Length;
}

// void IR_Input_GetDisplayString(char *Display_String, bit Blink_State)
// {
// 	unsigned char i;
// 	for (i = 0; i < Current_Input.Target_Length; i++)
// 	{
// 		if (i == Current_Input.Current_Cursor && Blink_State)
// 		{
// 			Display_String[i] = ' '; // 光标位置显示空格以实现闪烁效果
// 		}
// 		else
// 		{
// 			Display_String[i] = '0' + Current_Input.Digit_Buffer[i]; // 转换为字符
// 		}
// 	}
// 	Display_String[Current_Input.Target_Length] = '\0'; // 字符串结束符
// }

void Calendar_Set(void){
	static bit Cal_Set_Init_Flag = 0; // 0:未初始化, 1:已初始化
	unsigned char i;
	unsigned char ir_digit;
	xdata char Set_L1[LINE_MAX_LENGTH]=""; // 20YY-MM-DD
    xdata char Set_L2[LINE_MAX_LENGTH]=""; // HH:mm:ss

	// DS1302 读取时间
	
	if (!Cal_Set_Init_Flag) {
		DS1302_ReadTime(); // 从DS1302读取时间
		for (i = 0; i < 7; i++){
			Plause_Time[i] = DS1302_Time[i];
		}
		IR_Input_Init(CALENDAR_INPUT_LEN, Plause_Time); // 初始化红外输入模块，目标长度为14，初始值为当前时间
		Cal_Set_Init_Flag = 1;
	}
	// ir_digit = IR_Cmd_To_Digit(IR_Cmd);
	// if (ir_digit != 0xFF) { // 仅当红外输入是有效数字时才处理
	// 	IR_Input_Proc(ir_digit); // 处理红外输入
	// 	IR_Cmd = 0; // 清除命令，防止重复处理
	// }
	


	// // 闪烁显示设置
	// if (Flag_500ms_Task) {
	// 	Flag_500ms_Task = 0;
	// }
	
	// // 将Current_Input.Digit_Buffer转换为存储时间（两位合成一位）0-0/1 1-1/2 
	// for (i = 0; i < 6; i++)
	// {
	// 	Set_Time[i] = (Current_Input.Digit_Buffer[i * 2] << 4) | Current_Input.Digit_Buffer[i * 2 + 1];
	// }
	// Set_Time[6] = 0; // 星期默认设置为0

	// IR_Input_GetDisplayString(Set_L1, Flag_500ms_Task);

	// IR_Input_GetDisplayString(Set_L2, Flag_500ms_Task);
	// // 格式化显示字符串
	// sprintf(L1_String,"20%s-%s-%s      ",
	// 	&Set_L1[0], &Set_L1[2], &Set_L1[4]
	// );
	// LCD_ShowString(1,1,L1_String);
	// // 显示时间
	// sprintf(L2_String,"%s:%s:%s",
	// 	&Set_L2[0], &Set_L2[2], &Set_L2[4]
	// );
	// LCD_ShowString(2,1,L2_String);

	sprintf(Set_L1,"20%02u-%02u-%02u      ",
		Current_Input.Digit_Buffer[0],
		Current_Input.Digit_Buffer[1],
		Current_Input.Digit_Buffer[2]
	);
	LCD_ShowString(1,1,Set_L1);
	sprintf(Set_L2,"%02u:%02u:%02u",
		Current_Input.Digit_Buffer[3],
		Current_Input.Digit_Buffer[4],
		Current_Input.Digit_Buffer[5]
	);
	LCD_ShowString(2,1,Set_L2);



	// sprintf(Set_L1,"20%02u-%02u-%02u      ",
	// 	Set_Time[0], Set_Time[1], Set_Time[2]
	// );
	// LCD_ShowString(1,1,Set_L1);
	// sprintf(Set_L2,"%02u:%02u:%02u",
	// 	Set_Time[3], Set_Time[4], Set_Time[5]
	// );
	// LCD_ShowString(2,1,Set_L2);

	if (Current_Input.Input_Finished)
	{
		// 输入完成，更新 DS1302 时间
		for(i=0; i<7; i++){
			DS1302_Time[i] = Set_Time[i];
		}
		DS1302_SetTime(); // 设置时间

		// // 备份时间到 EEPROM
		// for(i=0; i<7; i++){
		// 	AT24C02_WriteByte(0x00 + i, DS1302_Time[i]); 
		// 	Delay(5);
		// }
		Cal_Set_Init_Flag = 0; // 重新初始化，准备下一次设置
	}
	
}

/*Module Code End*/

/*Proc Code*/
// DS18B20 状态机处理函数 (非阻塞)
void Temp_Proc(void){
    if (mode != Calendar) return;

    if (Temp_State == 0) { // Idle 状态
        if (Flag_5s_Task) {
            // 开始温度转换，不阻塞
			P3_5 = 1;
            DS18B20_ConvertT(); // 假设这个函数只是发送启动命令
            Flag_5s_Task = 0;
            Temp_State = 1; // 进入转换中状态
        }
    } else if (Temp_State == 1) { // 转换中状态
        if (Flag_750ms_Ready) {
			float temp;
            // 750ms 等待完成，读取温度
			P3_5 = 1;
			temp = DS18B20_ReadT();
			Temperature_Point = (int)(temp * 100) % 100;
			Temperature = (int)temp;

            Flag_750ms_Ready = 0;
            Temp_State = 0; // 回到 Idle 状态
        }
    }
}

void Calendar_Proc(void){
	static bit Cal_Mode = 0; // 0:Display, 1:Set
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
        // 100ms任务：时间读取和显示
        if (Flag_100ms_Task) {
            Calendar_Disp();
			// Time_Backup(); // 每秒备份时间到 EEPROM
			Flag_100ms_Task = 0; 
        }
		if (Flag_1s_Task) {
			Time_Backup(); // 每秒备份时间到 EEPROM
			Flag_1s_Task = 0;
		}
        
        // 5秒任务：温度转换和读取 (通过 Temp_Proc 状态机处理)
        Temp_Proc();        
	}
}

void Safe_Proc(void){
	if (mode != Safe) {return;} 
}

void Calculator_Proc(void){
	if (mode != Calculator) {return;}
}
/*Proc Code End*/

void DS18B20_Test(void)
{
    float temp_value;
    char temp_string[16];
    
    // 1. 启动温度转换
    DS18B20_ConvertT();
    
    // 2. 等待转换完成 (DS18B20需要约750ms)
    Delay(750);
    
    // 3. 读取温度值
    temp_value = DS18B20_ReadT();
    
    // 4. 将温度值格式化为字符串
    //    注意：Keil C51的sprintf默认可能不支持%f浮点数格式
    //    如果显示不正常，需要手动转换或更改编译器设置
    sprintf(temp_string, "Temp: %.2f C", temp_value);
    
    // 5. 在LCD上显示结果
    LCD_ShowString(1, 1, "DS18B20 Test:");
    LCD_ShowString(2, 1, "                "); // 先清空第二行
    LCD_ShowString(2, 1, temp_string);
}

void main()
{
	AT24C02_clearTime();  // 清除 EEPROM 中的时间数据，强制重新初始化
	DS18B20_ConvertT();
	Delay(750);
	Temperature = (int)DS18B20_ReadT();
	Temperature_Point = (int)(DS18B20_ReadT() * 100) % 100;
	Calendar_Init();
	LCD_Init();
	IR_Init();
	Motor_Init();
	// // LCD1602_Test();
	while(1)
	{
		IR_Data_Proc();
		Mode_Change();

		Calendar_Proc();
		Safe_Proc();
		Calculator_Proc();

		// DS18B20_Test();
        // Delay(1000); // 每隔1秒测试一次
	}
}

