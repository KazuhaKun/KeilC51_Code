// --- START OF FILE main.c ---

#include <REGX52.H>
#include "Delay.h"
#include "Motor.h"
#include "IR.h"
#include "LCD1602.h"
#include "Buzzer.h"
#include "DS1302.h"
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

bit Cal_Mode = 0; // 0:Display, 1:Set

/*===== 密码相关变量 =====*/
#define PASSWORD_LENGTH 8  // 密码长度为8位
xdata unsigned char Saved_Password[PASSWORD_LENGTH];  // 保存的密码
xdata unsigned char Input_Password[PASSWORD_LENGTH];   // 输入的密码
bit Safe_Mode = 0;  // 0:验证密码, 1:设置密码
bit Password_Verified = 0;  // 密码验证成功标志
bit Password_Error = 0;     // 密码错误标志

// EEPROM地址定义
#define EEPROM_PASSWORD_ADDR 0x10  // 密码存储起始地址(0x10-0x17)
#define EEPROM_PASSWORD_INIT_FLAG 0x18  // 密码初始化标志地址

/*===== 计算器相关变量 =====*/
typedef enum {
    CALC_INPUT_NUM1,    // 输入第一个数
    CALC_SELECT_OP,     // 选择运算符
    CALC_INPUT_NUM2,    // 输入第二个数
    CALC_SHOW_RESULT    // 显示结果
} Calc_State;

xdata Calc_State Calc_Current_State = CALC_INPUT_NUM1;
xdata long Calc_Num1 = 0;
xdata long Calc_Num2 = 0;
xdata unsigned char Calc_Operator = 0;
xdata float Calc_Result = 0.0;
xdata unsigned char Calc_Input_Count = 0;

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

void IR_Data_Proc(void)
{
	if (IR_GetDataFlag()){IR_Cmd=IR_GetCommand();}
}

/**
 * @brief 系统重置函数（按POWER键触发）
 * @param 无
 * @retval 无
 * @note 重置内容：
 *       1. 模式重置为万年历模式
 *       2. 时间恢复到2019年12月31日23:59:00
 *       3. 密码重置为00000000
 */
void System_Reset(void)
{
    unsigned char i;
    
    // 1. 重置模式为万年历
    mode = Calendar;
    Cal_Mode = 0;  // 显示模式
    
    // 2. 重置时间为 2019-12-31 23:59:00
    DS1302_Time[0] = 19;  // 年
    DS1302_Time[1] = 12;  // 月
    DS1302_Time[2] = 31;  // 日
    DS1302_Time[3] = 23;  // 时
    DS1302_Time[4] = 59;  // 分
    DS1302_Time[5] = 0;   // 秒
    DS1302_Time[6] = 2;   // 星期二
    
    // 设置时间到DS1302
    DS1302_SetTime();
    
    // 备份时间到EEPROM
    for(i = 0; i < 7; i++){
        AT24C02_WriteByte(0x00 + i, DS1302_Time[i]); 
        Delay(5);
    }
    
    // 3. 重置密码为 00000000
    for(i = 0; i < PASSWORD_LENGTH; i++){
        Saved_Password[i] = 0;  // 全部设为0
        AT24C02_WriteByte(EEPROM_PASSWORD_ADDR + i, 0);
        Delay(5);
    }
    
    // 4. 显示重置完成信息
    LCD_ShowString(1, 1, "System Reset!   ");
    LCD_ShowString(2, 1, "Time&PWD Reset  ");
    Delay(2000);  // 显示2秒
    
    // 5. 清除屏幕，准备显示万年历
    LCD_Init();
}

void Mode_Change(void){
	if (IR_Cmd == IR_MODE){
		mode = (Mode)((mode + 1) % 3);
		IR_Cmd = 0;
	}else if (IR_Cmd == IR_POWER){
		// 调用系统重置函数
		System_Reset();
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

/*===== 密码功能函数 =====*/

/**
 * @brief 初始化密码系统
 * 从EEPROM读取密码，如果未初始化则设置默认密码12345678
 */
void Password_Init(void) {
    unsigned char i;
    unsigned char init_flag;
    
    // 读取初始化标志
    init_flag = AT24C02_ReadByte(EEPROM_PASSWORD_INIT_FLAG);
    
    if (init_flag != 0xAA) {
        // 首次使用，设置默认密码 12345678
        for (i = 0; i < PASSWORD_LENGTH; i++) {
            Saved_Password[i] = (i + 1) % 10; // 12345678
            AT24C02_WriteByte(EEPROM_PASSWORD_ADDR + i, Saved_Password[i]);
            Delay(5);
        }
        // 设置初始化标志
        AT24C02_WriteByte(EEPROM_PASSWORD_INIT_FLAG, 0xAA);
        Delay(5);
    } else {
        // 从EEPROM读取密码
        for (i = 0; i < PASSWORD_LENGTH; i++) {
            Saved_Password[i] = AT24C02_ReadByte(EEPROM_PASSWORD_ADDR + i);
        }
    }
}

/**
 * @brief 保存密码到EEPROM
 */
void Password_Save(void) {
    unsigned char i;
    for (i = 0; i < PASSWORD_LENGTH; i++) {
        AT24C02_WriteByte(EEPROM_PASSWORD_ADDR + i, Saved_Password[i]);
        Delay(5);
    }
}

/**
 * @brief 验证密码是否正确
 * @return 1:正确, 0:错误
 */
bit Password_Verify(void) {
    unsigned char i;
    for (i = 0; i < PASSWORD_LENGTH; i++) {
        if (Input_Password[i] != Saved_Password[i]) {
            return 0; // 密码错误
        }
    }
    return 1; // 密码正确
}

/**
 * @brief 显示密码输入界面
 * @param is_setting 1:设置密码, 0:验证密码
 */
void Password_Display(bit is_setting) {
    unsigned char i;
    
    // 第一行显示提示
    if (is_setting) {
        LCD_ShowString(1, 1, "Set Password:   ");
    } else {
        LCD_ShowString(1, 1, "Enter Password: ");
    }
    
    // 第二行显示已输入的密码（用*或数字显示）
    for (i = 0; i < PASSWORD_LENGTH; i++) {
        if (i < Current_Input.Current_Cursor) {
            // 已输入的位置显示星号
            LCD_ShowChar(2, i + 1, '*');
        } else if (i == Current_Input.Current_Cursor && Flag_500ms_Task) {
            // 当前光标位置闪烁
            LCD_ShowChar(2, i + 1, '_');
        } else {
            // 未输入的位置显示下划线
            LCD_ShowChar(2, i + 1, '-');
        }
    }
    // 清除多余字符
    for (i = PASSWORD_LENGTH + 1; i <= 16; i++) {
        LCD_ShowChar(2, i, ' ');
    }
}

/**
 * @brief 步进电机正转5圈后反转5圈
 */
void Motor_OpenDoor(void) {
    LCD_ShowString(1, 1, "Door Opening... ");
    LCD_ShowString(2, 1, "Motor Forward.. ");
    
    // 调用步进电机开门函数（正转5圈后反转5圈）
    Stepper_Motor_OpenDoor();
    
    LCD_ShowString(1, 1, "Door Opened!    ");
    LCD_ShowString(2, 1, "Press Key...    ");
    Delay(2000); // 显示2秒
}

/**
 * @brief 蜂鸣器报警3秒
 */
void Buzzer_Alarm(void) {
    LCD_ShowString(1, 1, "Wrong Password! ");
    LCD_ShowString(2, 1, "Buzzer Alarm... ");
    
    // 调用蜂鸣器报警3秒函数
    Buzzer_Alarm_3s();
    
    // 显示完成信息
    LCD_ShowString(2, 1, "Alarm Stopped!  ");
    Delay(1000); // 显示1秒
}

/*===== 计算器功能函数 =====*/

void Calculator_Reset(void) {
    Calc_Num1 = 0;
    Calc_Num2 = 0;
    Calc_Operator = 0;
    Calc_Result = 0.0;
    Calc_Input_Count = 0;
    Calc_Current_State = CALC_INPUT_NUM1;
}

/**
 * @brief 显示数字到LCD（支持最多8位）
 * @param num 要显示的数字
 * @param line LCD行号(1或2)
 * @param start_col 起始列号
 */
void Calculator_ShowNumber(long num, unsigned char line, unsigned char start_col) {
    unsigned char i;
    unsigned char digits[8];
    unsigned char digit_count = 0;
    long temp = num;
    bit is_negative = 0;
    
    // 处理负数
    if (num < 0) {
        is_negative = 1;
        temp = -num;
    }
    
    // 提取每一位数字
    if (temp == 0) {
        digits[0] = 0;
        digit_count = 1;
    } else {
        while (temp > 0 && digit_count < 8) {
            digits[digit_count++] = temp % 10;
            temp /= 10;
        }
    }
    
    // 显示负号
    if (is_negative) {
        LCD_ShowChar(line, start_col++, '-');
    }
    
    // 反向显示数字
    for (i = digit_count; i > 0; i--) {
        LCD_ShowChar(line, start_col++, '0' + digits[i-1]);
    }
}

void Calculator_ShowResult(float result) {
    long integer_part;
    unsigned int decimal_part;
    unsigned char pos;
    unsigned char i;
    
    LCD_ShowString(1, 1, "Res:            ");
    
    pos = 1;
    
    // 处理负数
    if (result < 0) {
        LCD_ShowChar(2, pos++, '-');
        result = -result;
    }
    
    integer_part = (long)result;
    decimal_part = (unsigned int)((result - integer_part) * 10000);
    
    // 显示整数部分
    Calculator_ShowNumber(integer_part, 2, pos);
    
    // 计算整数部分占用的位置
    if (integer_part == 0) {
        pos += 1;
    } else {
        long temp = integer_part;
        while (temp > 0) {
            temp /= 10;
            pos++;
        }
    }
    
    // 只在除法时显示小数部分
    if (Calc_Operator == 4 && pos < 11) {
        LCD_ShowChar(2, pos++, '.');
        LCD_ShowChar(2, pos++, '0' + (decimal_part / 1000) % 10);
        LCD_ShowChar(2, pos++, '0' + (decimal_part / 100) % 10);
        LCD_ShowChar(2, pos++, '0' + (decimal_part / 10) % 10);
        LCD_ShowChar(2, pos++, '0' + decimal_part % 10);
    }
    
    // 清除第二行剩余部分
    for (i = pos; i <= 16; i++) {
        LCD_ShowChar(2, i, ' ');
    }
}

void Calculator_Calculate(void) {
    switch (Calc_Operator) {
        case 1:
            Calc_Result = (float)Calc_Num1 + (float)Calc_Num2;
            break;
        case 2:
            Calc_Result = (float)Calc_Num1 - (float)Calc_Num2;
            break;
        case 3:
            Calc_Result = (float)Calc_Num1 * (float)Calc_Num2;
            break;
        case 4:
            if (Calc_Num2 != 0) {
                Calc_Result = (float)Calc_Num1 / (float)Calc_Num2;
            } else {
                LCD_ShowString(1, 1, "Err:Div by 0    ");
                LCD_ShowString(2, 1, "                ");
                Delay(1500);
                Calculator_Reset();
                return;
            }
            break;
        default:
            return;
    }
    
    Calculator_ShowResult(Calc_Result);
    Calc_Current_State = CALC_SHOW_RESULT;
}

/**
 * @brief 显示当前输入状态
 */
void Calculator_Display(void) {
    unsigned char i;
    
    switch (Calc_Current_State) {
        case CALC_INPUT_NUM1:
            LCD_ShowString(1, 1, "Num1:           ");
            if (Calc_Input_Count > 0) {
                Calculator_ShowNumber(Calc_Num1, 1, 6);
            }
            LCD_ShowString(2, 1, "                ");
            break;
            
        case CALC_SELECT_OP:
            LCD_ShowString(1, 1, "Select Op:      ");
            LCD_ShowString(2, 1, "+  -  *  /      ");
            break;
            
        case CALC_INPUT_NUM2:
            LCD_ShowString(1, 1, "Num2:           ");
            if (Calc_Input_Count > 0) {
                Calculator_ShowNumber(Calc_Num2, 1, 6);
            }
            // 显示运算符在第二行
            LCD_ShowChar(2, 1, Calc_Operator == 1 ? '+' : 
                               Calc_Operator == 2 ? '-' : 
                               Calc_Operator == 3 ? '*' : '/');
            // 清除第二行其余部分
            for (i = 2; i <= 16; i++) {
                LCD_ShowChar(2, i, ' ');
            }
            break;
            
        case CALC_SHOW_RESULT:
            // 结果已在 Calculate 函数中显示
            return;
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
    unsigned char i;
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
        if (i*2 < Target_Length && Initial_Values != 0) {
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

void Calendar_Set(void){
    unsigned char i;
    unsigned char ir_digit;
    static bit Set_Init_Flag = 0;
    
    // 初始化设置状态
    if (!Set_Init_Flag) {
        // 读取当前时间作为初始值
        DS1302_ReadTime();
        // 初始化输入模块，传入当前时间作为初始值
        IR_Input_Init(CALENDAR_INPUT_LEN, DS1302_Time);
        Set_Init_Flag = 1;
    }
    
    // 处理数字输入 (0-9)
    ir_digit = IR_Cmd_To_Digit(IR_Cmd);
    if (ir_digit != 0xFF) {
        IR_Input_Proc(ir_digit); // 存储数字并移动光标
        IR_Cmd = 0; // 清除命令
    }
    
    // 显示设置界面（带闪烁）
    if (Flag_100ms_Task) {
        // 第一行：20YY-MM-DD
        LCD_ShowString(1, 1, "Set:20");
        
        // 年 YY (索引0,1 -> 显示位置7,8)
        for (i = 0; i < 2; i++) {
            if (i + 0 == Current_Input.Current_Cursor && Current_Input.Blinking_State) {
                LCD_ShowChar(1, 7 + i, ' '); // 闪烁
            } else {
                LCD_ShowChar(1, 7 + i, '0' + Current_Input.Digit_Buffer[i]);
            }
        }
        LCD_ShowChar(1, 9, '-');
        
        // 月 MM (索引2,3 -> 显示位置10,11)
        for (i = 0; i < 2; i++) {
            if (i + 2 == Current_Input.Current_Cursor && Current_Input.Blinking_State) {
                LCD_ShowChar(1, 10 + i, ' ');
            } else {
                LCD_ShowChar(1, 10 + i, '0' + Current_Input.Digit_Buffer[2 + i]);
            }
        }
        LCD_ShowChar(1, 12, '-');
        
        // 日 DD (索引4,5 -> 显示位置13,14)
        for (i = 0; i < 2; i++) {
            if (i + 4 == Current_Input.Current_Cursor && Current_Input.Blinking_State) {
                LCD_ShowChar(1, 13 + i, ' ');
            } else {
                LCD_ShowChar(1, 13 + i, '0' + Current_Input.Digit_Buffer[4 + i]);
            }
        }
        
        // 清除第一行剩余部分
        LCD_ShowChar(1, 15, ' ');
        LCD_ShowChar(1, 16, ' ');
        
        // 第二行：HH:mm:ss
        // 时 HH (索引6,7 -> 显示位置1,2)
        for (i = 0; i < 2; i++) {
            if (i + 6 == Current_Input.Current_Cursor && Current_Input.Blinking_State) {
                LCD_ShowChar(2, 1 + i, ' ');
            } else {
                LCD_ShowChar(2, 1 + i, '0' + Current_Input.Digit_Buffer[6 + i]);
            }
        }
        LCD_ShowChar(2, 3, ':');
        
        // 分 mm (索引8,9 -> 显示位置4,5)
        for (i = 0; i < 2; i++) {
            if (i + 8 == Current_Input.Current_Cursor && Current_Input.Blinking_State) {
                LCD_ShowChar(2, 4 + i, ' ');
            } else {
                LCD_ShowChar(2, 4 + i, '0' + Current_Input.Digit_Buffer[8 + i]);
            }
        }
        LCD_ShowChar(2, 6, ':');
        
        // 秒 ss (索引10,11 -> 显示位置7,8)
        for (i = 0; i < 2; i++) {
            if (i + 10 == Current_Input.Current_Cursor && Current_Input.Blinking_State) {
                LCD_ShowChar(2, 7 + i, ' ');
            } else {
                LCD_ShowChar(2, 7 + i, '0' + Current_Input.Digit_Buffer[10 + i]);
            }
        }
        
        // 清除第二行剩余部分
        for (i = 9; i <= 16; i++) {
            LCD_ShowChar(2, i, ' ');
        }
        
        Flag_100ms_Task = 0;
    }
    
    // 处理输入完成（保存设置）
    if (Current_Input.Input_Finished) {
        // 将输入的12位数字转换为6个时间分量
        DS1302_Time[0] = Current_Input.Digit_Buffer[0] * 10 + Current_Input.Digit_Buffer[1];  // 年
        DS1302_Time[1] = Current_Input.Digit_Buffer[2] * 10 + Current_Input.Digit_Buffer[3];  // 月
        DS1302_Time[2] = Current_Input.Digit_Buffer[4] * 10 + Current_Input.Digit_Buffer[5];  // 日
        DS1302_Time[3] = Current_Input.Digit_Buffer[6] * 10 + Current_Input.Digit_Buffer[7];  // 时
        DS1302_Time[4] = Current_Input.Digit_Buffer[8] * 10 + Current_Input.Digit_Buffer[9];  // 分
        DS1302_Time[5] = Current_Input.Digit_Buffer[10] * 10 + Current_Input.Digit_Buffer[11]; // 秒
        
        // 星期自动计算（可选，这里简单设为1）
        DS1302_Time[6] = 1;
        
        // 设置时间到DS1302
        DS1302_SetTime();
        
        // 备份到EEPROM
        for (i = 0; i < 7; i++) {
            AT24C02_WriteByte(0x00 + i, DS1302_Time[i]);
            Delay(5);
        }
        
        // 显示保存成功提示
        LCD_ShowString(1, 1, "Time Saved!     ");
        LCD_ShowString(2, 1, "                ");
        Delay(1000);
        
        // 退出设置模式
        Cal_Mode = 0;
        Set_Init_Flag = 0;
        Current_Input.Input_Finished = 0;
        Current_Input.Target_Length = 0; // 重置目标长度
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
	if (mode != Calendar) {return;}
    
    if (IR_Cmd == IR_START_STOP) {
        Cal_Mode = !Cal_Mode; // 切换模式
        IR_Cmd = 0; // 清除命令，防止重复处理
    }
        
	// 2. 周期性任务处理
    if (Cal_Mode) {
        // 设置模式
        // --- A. 按键处理 (只处理保存键) ---
        if (IR_Cmd == IR_EQ) {
            Current_Input.Input_Finished = 1;
            IR_Cmd = 0; // 清除命令，防止重复处理
        }
        
        // --- B. 设置逻辑 (包括输入和保存) ---
		Calendar_Set();

        // 100ms 任务：用于刷新设置界面
        if (Flag_100ms_Task) {
            // 在Calendar_Set中处理
        }

        // 500ms 任务：用于闪烁切换
        if (Flag_500ms_Task) {
            Current_Input.Blinking_State = !Current_Input.Blinking_State;
            Flag_100ms_Task = 1; // 触发界面刷新
            Flag_500ms_Task = 0;
        }
        
	} else {
        // 显示模式
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
    static bit Safe_Init_Flag = 0;
    unsigned char ir_digit;
    
	if (mode != Safe) {
        Safe_Init_Flag = 0;
        return;
    } 
    
    // 初始化
    if (!Safe_Init_Flag) {
        // 初始化输入模块
        IR_Input_Init(PASSWORD_LENGTH, 0); // 8位密码，不预填充
        Safe_Mode = 0; // 默认为验证密码模式
        Password_Verified = 0;
        Password_Error = 0;
        Safe_Init_Flag = 1;
        Flag_100ms_Task = 1; // 立即刷新显示
    }
    
    // 按键处理
    if (IR_Cmd == IR_START_STOP) {
        // 切换设置/验证模式
        Safe_Mode = !Safe_Mode;
        IR_Input_Init(PASSWORD_LENGTH, 0); // 重新初始化输入
        Password_Verified = 0;
        Password_Error = 0;
        IR_Cmd = 0;
        Flag_100ms_Task = 1;
    }
    
    // 处理数字输入
    ir_digit = IR_Cmd_To_Digit(IR_Cmd);
    if (ir_digit != 0xFF) {
        // 存储输入的数字
        if (Safe_Mode) {
            // 设置密码模式
            Saved_Password[Current_Input.Current_Cursor] = ir_digit;
        } else {
            // 验证密码模式
            Input_Password[Current_Input.Current_Cursor] = ir_digit;
        }
        
        IR_Input_Proc(ir_digit); // 移动光标
        IR_Cmd = 0;
        Flag_100ms_Task = 1;
    }
    
    // 处理确认键
    if (IR_Cmd == IR_EQ) {
        Current_Input.Input_Finished = 1;
        IR_Cmd = 0;
    }
    
    // 周期性刷新显示
    if (Flag_100ms_Task) {
        Password_Display(Safe_Mode);
        Flag_100ms_Task = 0;
    }
    
    // 输入完成处理
    if (Current_Input.Input_Finished) {
        if (Safe_Mode) {
            // 设置密码模式：保存密码
            Password_Save();
            LCD_ShowString(1, 1, "Password Saved! ");
            LCD_ShowString(2, 1, "                ");
            Delay(1500);
            Safe_Mode = 0; // 切换回验证模式
        } else {
            // 验证密码模式
            if (Password_Verify()) {
                // 密码正确
                Password_Verified = 1;
                LCD_ShowString(1, 1, "Password OK!    ");
                LCD_ShowString(2, 1, "Opening Door... ");
                Delay(1000);
                
                // 调用步进电机开门函数
                Motor_OpenDoor();
                
                Password_Verified = 0;
            } else {
                // 密码错误
                Password_Error = 1;
                
                // 调用蜂鸣器报警函数
                Buzzer_Alarm();
                
                Password_Error = 0;
            }
        }
        
        // 重新初始化输入
        IR_Input_Init(PASSWORD_LENGTH, 0);
        Current_Input.Input_Finished = 0;
        Flag_100ms_Task = 1;
    }
}

void Calculator_Proc(void){
    static bit Calc_Init_Flag = 0;
    unsigned char ir_digit;
    
	if (mode != Calculator) {
        Calc_Init_Flag = 0;
        return;
    }
    
    // 初始化
    if (!Calc_Init_Flag) {
        Calculator_Reset();
        Calc_Init_Flag = 1;
        Flag_100ms_Task = 1;
    }
    
    // 处理按键
    switch (Calc_Current_State) {
        case CALC_INPUT_NUM1:
            // 输入第一个数字（最多8位或乘法时4位）
            ir_digit = IR_Cmd_To_Digit(IR_Cmd);
            if (ir_digit != 0xFF) {
                Calc_Num1 = Calc_Num1 * 10 + ir_digit;
                Calc_Input_Count++;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            
            // 按EQ确认输入，进入选择运算符状态
            if (IR_Cmd == IR_EQ) {
                Calc_Current_State = CALC_SELECT_OP;
                Calc_Input_Count = 0;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            
            // 按VOL-清除当前输入
            if (IR_Cmd == IR_VOL_MINUS) {
                Calc_Num1 = 0;
                Calc_Input_Count = 0;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            break;
            
        case CALC_SELECT_OP:
            // 选择运算符
            if (IR_Cmd == IR_VOL_ADD) {
                Calc_Operator = 1; // 加法
                Calc_Current_State = CALC_INPUT_NUM2;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            } else if (IR_Cmd == IR_VOL_MINUS) {
                Calc_Operator = 2; // 减法
                Calc_Current_State = CALC_INPUT_NUM2;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            } else if (IR_Cmd == IR_1) {
                Calc_Operator = 3; // 乘法（用1键代替*）
                Calc_Current_State = CALC_INPUT_NUM2;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            } else if (IR_Cmd == IR_2) {
                Calc_Operator = 4; // 除法（用2键代替/）
                Calc_Current_State = CALC_INPUT_NUM2;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            
            // 按START_STOP返回重新输入第一个数
            if (IR_Cmd == IR_START_STOP) {
                Calculator_Reset();
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            break;
            
        case CALC_INPUT_NUM2:
            // 输入第二个数字
            ir_digit = IR_Cmd_To_Digit(IR_Cmd);
            if (ir_digit != 0xFF) {
                // 乘法限制为4位
                if (Calc_Operator == 3 && Calc_Input_Count >= 4) {
                    IR_Cmd = 0;
                    break;
                }
                // 其他运算限制为8位
                if (Calc_Operator != 3 && Calc_Input_Count >= 8) {
                    IR_Cmd = 0;
                    break;
                }
                
                Calc_Num2 = Calc_Num2 * 10 + ir_digit;
                Calc_Input_Count++;
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            
            // 按EQ计算结果
            if (IR_Cmd == IR_EQ) {
                Calculator_Calculate();
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            
            // 按START_STOP返回重新输入
            if (IR_Cmd == IR_START_STOP) {
                Calculator_Reset();
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            break;
            
        case CALC_SHOW_RESULT:
            // 显示结果后，按任意数字键开始新的计算
            ir_digit = IR_Cmd_To_Digit(IR_Cmd);
            if (ir_digit != 0xFF || IR_Cmd == IR_START_STOP) {
                Calculator_Reset();
                // 如果是数字键，将其作为第一个数的第一位
                if (ir_digit != 0xFF) {
                    Calc_Num1 = ir_digit;
                    Calc_Input_Count = 1;
                }
                IR_Cmd = 0;
                Flag_100ms_Task = 1;
            }
            break;
    }
    
    // 周期性刷新显示
    if (Flag_100ms_Task) {
        Calculator_Display();
        Flag_100ms_Task = 0;
    }
}

void main()
{
// ... (保持原样)
	// AT24C02_clearTime();  // 清除 EEPROM 中的时间数据，强制重新初始化 - 注释掉以保留设置的时间
	DS18B20_ConvertT();
	Delay(750);
	Temperature = (int)DS18B20_ReadT();
	Temperature_Point = (int)(DS18B20_ReadT() * 100) % 100;
	Calendar_Init();
	Password_Init();  // 初始化密码系统
	LCD_Init();
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