// --- START OF FILE main.c ---

#include <REGX52.H>
#include "Delay.h"
#include "Key.h"
#include "Nixie.h"
#include "Motor.h"
#include "IR.h"
#include "LCD1602.h"
#include "Buzzer.h"
#include <STDIO.H> // **移除或注释掉此行以节省内存，因为我们不再使用 sprintf**
#include "DS1302.h"
// #include "string.h" // **移除或注释掉此行以节省内存，因为我们不再使用 string.h 函数**
#include "DS18B20.h"
#include "AT24C02.h"
#include "Timer0.h"
#include "Timer1.h"

/*PD*/
#define LINE_MAX_LENGTH 16
// 目标输入长度：年(2)月(2)日(2)时(2)分(2)秒(2) = 12位数字
#define CALENDAR_INPUT_LEN 12 

/*PD End*/
/*PV*/
// ... (保持原样)
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
// ... (AT24C02_clearTime, LCD1602_Test, IR_Data_Proc, Mode_Change, IR_Cmd_To_Digit 保持原样)

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

// **修改：使用手动ASCII转换，移除 sprintf**
void Calendar_Disp(void){
    unsigned char i, j;
	DS1302_ReadTime(); // 从DS1302读取时间
    
    // ====== L1: 20YY-MM-DD (16 chars) ======
    L1_String[0] = '2'; L1_String[1] = '0';
    L1_String[4] = '-'; L1_String[7] = '-';
    
    // YY (i=0, L1_String index 2,3)
    L1_String[2] = DS1302_Time[0]/10 + '0';
    L1_String[3] = DS1302_Time[0]%10 + '0';
    
    // MM (i=1, L1_String index 5,6)
    L1_String[5] = DS1302_Time[1]/10 + '0';
    L1_String[6] = DS1302_Time[1]%10 + '0';
    
    // DD (i=2, L1_String index 8,9)
    L1_String[8] = DS1302_Time[2]/10 + '0';
    L1_String[9] = DS1302_Time[2]%10 + '0';
    
    // 填充剩余空格
    for (i = 10; i < LINE_MAX_LENGTH; i++) { L1_String[i] = ' '; }
    L1_String[LINE_MAX_LENGTH-1] = '\0';
    
	LCD_ShowString(1,1,L1_String);

    // ====== L2: HH:mm:ss  Temp.Temp_Point C (16 chars) ======
    L2_String[2] = ':'; L2_String[5] = ':';
    
    // HH (i=3, L2_String index 0,1)
    L2_String[0] = DS1302_Time[3]/10 + '0';
    L2_String[1] = DS1302_Time[3]%10 + '0';
    
    // mm (i=4, L2_String index 3,4)
    L2_String[3] = DS1302_Time[4]/10 + '0';
    L2_String[4] = DS1302_Time[4]%10 + '0';
    
    // ss (i=5, L2_String index 6,7)
    L2_String[6] = DS1302_Time[5]/10 + '0';
    L2_String[7] = DS1302_Time[5]%10 + '0';

    L2_String[8] = ' ';
    L2_String[9] = ' ';
    
    // Temp (L2_String index 10,11)
    // 假设 Temperature <= 99
    L2_String[10] = Temperature/10 + '0';
    L2_String[11] = Temperature%10 + '0';
    
    L2_String[12] = '.';
    
    // Temp_Point (L2_String index 13,14)
    L2_String[13] = Temperature_Point/10 + '0';
    L2_String[14] = Temperature_Point%10 + '0';
    
    L2_String[15] = 'C';
    L2_String[LINE_MAX_LENGTH] = '\0'; // 确保字符串正确终止
    
	LCD_ShowString(2,1,L2_String);
}

void Time_Backup(void){
// ... (保持原样)
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

    Current_Input.Target_Length = Target_Length; // 目标长度：12
    Current_Input.Current_Cursor = 0; // 光标从第一位开始
    Current_Input.Input_Finished = 0; // 输入未完成

    // 循环6次，对应6个时间分量 (年,月,日,时,分,秒)
    for (i = 0; i < 6; i++) 
    {
        // Target_Length 是 12。我们检查 i*2 是否在范围内
        if (i*2 < Target_Length && Initial_Values != NULL) {
            // 将两位数（例如 19）拆分成两个数字（1 和 9）
            Current_Input.Digit_Buffer[i*2] = Initial_Values[i]/10;
			Current_Input.Digit_Buffer[i*2+1] = Initial_Values[i]%10;
        } else {
            // 否则初始化为 0
            Current_Input.Digit_Buffer[i*2] = 0; 
			Current_Input.Digit_Buffer[i*2+1] = 0;
        }
    }
}

void IR_Input_Proc(unsigned char ir_digit){
	if(Current_Input.Target_Length == 0 || Current_Input.Input_Finished){
		return; // 如果没有设置目标长度或输入已完成，直接返回
	}
	
    // 1. 存储新的数字
	Current_Input.Digit_Buffer[Current_Input.Current_Cursor] = ir_digit;

    // 2. 移动光标
	Current_Input.Current_Cursor++;

    // 3. 检查是否完成输入
    if (Current_Input.Current_Cursor >= Current_Input.Target_Length) {
        Current_Input.Input_Finished = 1; // 12位输入完成
        Current_Input.Current_Cursor = 0; // 光标归零
    }
}

// **新增/修改：使用手动索引和赋值，移除 strlen**
void Calendar_Build_String_Set(bit Blink_State)
{
    unsigned char i;
    unsigned char digit;
    unsigned char L1_Cursor = 0;
    unsigned char L2_Cursor = 0;
    
    // ====== L1: 20YY-MM-DD (16 chars) ======
    // 20
    L1_String[L1_Cursor++] = '2'; 
    L1_String[L1_Cursor++] = '0';
    
    for (i = 0; i < 6; i++) // Digits 0 to 5 (YY MM DD)
    {
        digit = Current_Input.Digit_Buffer[i];
        
        // 闪烁逻辑: 如果当前光标位置 i 且 Blink_State 为真，则显示空格
        if (i == Current_Input.Current_Cursor && Blink_State) {
            L1_String[L1_Cursor++] = ' ';
        } else {
            L1_String[L1_Cursor++] = '0' + digit;
        }
        
        // 添加分隔符 (在 YY2 (i=1) 和 MM2 (i=3) 之后)
        if (i == 1 || i == 3) {
            L1_String[L1_Cursor++] = '-';
        }
    }
    // 填充L1长度
    while (L1_Cursor < LINE_MAX_LENGTH) { L1_String[L1_Cursor++] = ' '; }
    L1_String[LINE_MAX_LENGTH-1] = '\0'; // 字符串结束符

    // ====== L2: HH:mm:ss (16 chars) ======
    for (i = 6; i < 12; i++) // Digits 6 to 11 (HH mm ss)
    {
        digit = Current_Input.Digit_Buffer[i];

        // 闪烁逻辑
        if (i == Current_Input.Current_Cursor && Blink_State) {
            L2_String[L2_Cursor++] = ' ';
        } else {
            L2_String[L2_Cursor++] = '0' + digit;
        }

        // 添加分隔符 (在 HH2 (i=7) 和 mm2 (i=9) 之后)
        if (i == 7 || i == 9) {
            L2_String[L2_Cursor++] = ':';
        }
    }
    // 填充L2长度
    while (L2_Cursor < LINE_MAX_LENGTH) { L2_String[L2_Cursor++] = ' '; }
    L2_String[LINE_MAX_LENGTH-1] = '\0'; // 字符串结束符
}


void Calendar_Set(void){
	static bit Cal_Set_Init_Flag = 0; // 0:未初始化, 1:已初始化
	unsigned char i;
	unsigned char ir_digit;

	
	if (!Cal_Set_Init_Flag) {
		DS1302_ReadTime(); // 从DS1302读取时间
		for (i = 0; i < 7; i++){
			Plause_Time[i] = DS1302_Time[i];
		}
		IR_Input_Init(CALENDAR_INPUT_LEN, Plause_Time); // 初始化红外输入模块，目标长度为12，初始值为当前时间
		Cal_Set_Init_Flag = 1;
        
        Flag_100ms_Task = 1; // 立即刷新显示
	}
    
    // 1. 处理红外输入
	ir_digit = IR_Cmd_To_Digit(IR_Cmd);
	if (ir_digit != 0xFF) { // 仅当红外输入是有效数字时才处理
		IR_Input_Proc(ir_digit); // 处理红外输入：存入Digit_Buffer，移动光标
		IR_Cmd = 0; // 清除命令，防止重复处理
        Flag_100ms_Task = 1; // 立即刷新显示
	}
    // // 处理确认键 (例如 IR_OK) - 强制完成输入并保存
    // if (IR_Cmd == IR_EQ) {
    //     Current_Input.Input_Finished = 1;
    //     IR_Cmd = 0;
    // }
	
    // 2. 周期性刷新显示 (100ms 任务)
    if (Flag_100ms_Task) {
        
        // 3. 处理显示：格式化字符串并在光标位置应用闪烁
        Calendar_Build_String_Set(Flag_500ms_Task); // **修改：使用新的无 strlen 函数**

        // 显示字符串
        LCD_ShowString(1,1,L1_String);
        LCD_ShowString(2,1,L2_String);
        
        Flag_100ms_Task = 0;
    }


	// 4. 输入完成后的数据转换和保存
	if (Current_Input.Input_Finished)
	{
		// 输入完成，将 12 位数字重新组合成 6 个时间分量
		for (i = 0; i < 6; i++)
		{
            // Set_Time[0]=D[0]*10+D[1], Set_Time[1]=D[2]*10+D[3], ...
			Set_Time[i] = Current_Input.Digit_Buffer[i * 2] * 10 + Current_Input.Digit_Buffer[i * 2 + 1];
		}
        // 星期保持不变
        Set_Time[6] = Plause_Time[6]; 

		// 更新 DS1302 时间
		for(i=0; i<7; i++){
			DS1302_Time[i] = Set_Time[i];
		}
		DS1302_SetTime(); // 设置时间

		Cal_Set_Init_Flag = 0; // 重置初始化标志，下次进入重新加载时间
        Current_Input.Input_Finished = 0; // 重置输入完成标志
	}
	
}

/*Module Code End*/

/*Proc Code*/
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
	unsigned char ir_cmd_temp = 0; // 临时存储 IR_Cmd

	if (mode != Calendar) {return;}
    
    // 1. 暂存命令并清除 IR_Cmd，防止在 Calendar_Set 中被数字键误触发

    ir_cmd_temp = IR_Cmd;
    IR_Cmd = 0;
    
	// 2. 周期性任务处理
    if (Cal_Mode) {
        // 设置模式
        
        // --- A. 按键处理 (只处理退出键) ---
        if (ir_cmd_temp == IR_EQ) {
            // IR_EQ：保存并退出。首先标记完成，让 Calendar_Set 在本周期执行保存逻辑。
            Current_Input.Input_Finished = 1;
        } else if (ir_cmd_temp == IR_START_STOP) {
            // IR_START_STOP：不保存退出。
            Cal_Mode = 0; // 立即退出
            Flag_100ms_Task = 1; // 立即刷新
        } else {
            // 其他键，如果是数字键，交给 Calendar_Set 处理（IR_Cmd 已经被清空，这里需要恢复）
            IR_Cmd = ir_cmd_temp; // 恢复 IR_Cmd 给 Calendar_Set 处理数字输入
        }
        
        // --- B. 设置逻辑 (包括输入和保存) ---
		Calendar_Set();
        
        // --- C. 退出逻辑 (如果 Calendar_Set 刚刚完成保存) ---
        // Calendar_Set 在保存完成时会重置 Current_Input.Input_Finished 为 0。
        // 如果我们是按 IR_EQ 进来的，我们需要在保存完成后退出。
        if (ir_cmd_temp == IR_EQ && Cal_Mode == 1) { // 检查是否按了 IR_EQ，并且还没有退出
             // 检查保存是否成功完成（如果 Calendar_Set 重置了 Cal_Set_Init_Flag，则认为完成）
             // 最简单的就是按 IR_EQ 后强制退出。由于 Calendar_Set 已经在本周期运行，保存已经完成。
             Cal_Mode = 0;
             Flag_100ms_Task = 1; // 立即刷新
        }
        
        // 500ms 任务：用于闪烁切换
        if (Flag_500ms_Task) {
            Flag_500ms_Task = 0;
        }
        
	} else {
        // 显示模式
        
        // --- D. 按键处理 (只处理进入设置键) ---
        if (ir_cmd_temp == IR_START_STOP) {
            // 从显示模式进入设置模式
            Cal_Mode = 1;
            Flag_100ms_Task = 1; // 立即刷新
        }
        
        // --- E. 显示逻辑 ---
        if (Flag_100ms_Task) {
            Calendar_Disp();
			Flag_100ms_Task = 0; 
        }
        // ... (Time_Backup 和 Temp_Proc 保持不变)
		if (Flag_1s_Task) {
			Time_Backup(); // 每秒备份时间到 EEPROM
			Flag_1s_Task = 0;
		}
        
        // 5秒任务：温度转换和读取 (通过 Temp_Proc 状态机处理)
        Temp_Proc();        
	}
}
// ... (Safe_Proc, Calculator_Proc, DS18B20_Test 保持原样)
void Safe_Proc(void){
	if (mode != Safe) {return;} 
}

void Calculator_Proc(void){
	if (mode != Calculator) {return;}
}

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
    // sprintf(temp_string, "Temp: %.2f C", temp_value); // 移除
    
    // 5. 在LCD上显示结果
    LCD_ShowString(1, 1, "DS18B20 Test:");
    // LCD_ShowString(2, 1, "                "); // 先清空第二行
    // LCD_ShowString(2, 1, temp_string);
}

void main()
{
// ... (保持原样)
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