#include <REGX52.H>
#include "Timer1.h"
#include "main.h"
#include "Delay.h"

// 引脚定义 - 直流电机（原有的PWM控制）
sbit Motor=P1^0;

// 步进电机引脚定义 - 28BYJ-48 (使用P1口的低4位)
// 连接到ULN2003D的IN1-IN4
sbit Stepper_A = P1^0;  // 对应电机A相，连接ULN2003D的IN1
sbit Stepper_B = P1^1;  // 对应电机B相，连接ULN2003D的IN2
sbit Stepper_C = P1^2;  // 对应电机C相，连接ULN2003D的IN3
sbit Stepper_D = P1^3;  // 对应电机D相，连接ULN2003D的IN4

unsigned char Counter,Compare;

/**
  * @brief  电机初始化
  * @param  无
  * @retval 无
  */
void Motor_Init(void)
{
	Timer1_Init();
}

/**
  * @brief  电机设置速度
  * @param  Speed 要设置的速度，范围0~100
  * @retval 无
  */
void Motor_SetSpeed(unsigned char Speed)
{
	Compare=Speed;
}

//定时器1中断函数
void Timer1_Routine() interrupt 3
{
	static unsigned char SysTick_1s = 0; // 内部1秒计数（替代原来的SysTick）
	static unsigned char SysTick_100ms = 0; // 内部100毫秒计数
	static unsigned char SysTick_500ms = 0; // 内部500毫秒计数
	TL1 = 0xA4;		//设置定时初值
	TH1 = 0xFF;		//设置定时初值
	Counter++;
	Counter%=100;	//计数值变化范围限制在0~99

	// 检查 10ms 周期是否完成 (Counter==0 happens every 10ms)
	if(Counter == 0) {
		// 0. 100ms 任务计数 (基于 10ms)
		if (++SysTick_100ms >= 10) {
			SysTick_100ms = 0;
			Flag_100ms_Task = 1; // 100毫秒任务标志置位
		}
		// 0.5. 500ms 任务计数 (基于 10ms)
		if (++SysTick_500ms >= 50) {
			SysTick_500ms = 0;
			Flag_500ms_Task = 1; // 500毫秒任务标志置位
		}
		// 1. 系统 1s 计数 (100 * 10ms = 1000ms)
		if (++SysTick_1s >= 100) {
			SysTick_1s = 0;
			Flag_1s_Task = 1; // 1秒任务标志置位
            
			// 2. 5s 任务计数 (基于 1s)
			if (++Time_5s_Count >= 5) {
				Time_5s_Count = 0;
				Flag_5s_Task = 1; // 5秒任务标志置位
			}            
		}
        
        // 4. DS18B20 750ms 非阻塞等待 (75 * 10ms = 750ms)
        // 假设你在主循环启动转换后，会进入 Temp_State = 1，并开始计时
        if (Temp_State == 1) {
            static unsigned char Delay_750ms = 0;
            if (++Delay_750ms >= 75) {
                Delay_750ms = 0;
                Flag_750ms_Ready = 1; // 750ms 转换完成标志
            }
        }
	}

	if(Counter<Compare)	//计数值小于比较值
	{
		Motor=1;		//输出1
	}
	else				//计数值大于比较值
	{
		Motor=0;		//输出0
	}
}

/*========== 步进电机控制函数 ==========*/

// 四拍驱动时序数组（高扭矩模式）
// 对应 P1.3 P1.2 P1.1 P1.0 -> D C B A
code unsigned char Stepper_Code_4_Step[4] = {
    0x03, // 0011B (A, B ON)
    0x06, // 0110B (B, C ON)
    0x0C, // 1100B (C, D ON)
    0x09  // 1001B (D, A ON)
};

/**
  * @brief  步进电机旋转指定步数
  * @param  steps 步数（28BYJ-48一圈约512步，使用四拍模式）
  * @param  direction 方向，0-正转，1-反转
  * @retval 无
  */
void Stepper_Motor_Rotate(unsigned int steps, unsigned char direction)
{
    unsigned int i;
    static unsigned char step_index = 0; // 记录当前步序的索引
    
    for (i = 0; i < steps; i++)
    {
        if (direction == 0) // 正转
        {
            step_index++;
            if (step_index > 3) step_index = 0; // 循环索引 0->1->2->3->0
        }
        else // 反转
        {
            if (step_index == 0) step_index = 4; // 防止无符号减到负数
            step_index--;
            // 循环索引 0->3->2->1->0
        }
        
        // 将时序值输出到P1口的低4位（P1.0-P1.3）
        // 保留P1口高4位的状态
        P1 = (P1 & 0xF0) | Stepper_Code_4_Step[step_index];
        
        // 延时控制转速，2ms延时产生较快的转速
        Delay(2); // 使用已有的Delay函数（约2ms）
    }
    
    // 停止时关闭所有相，降低功耗和发热
    P1 = P1 & 0xF0; // 清除低4位
}

/**
  * @brief  步进电机开门动作（正转5圈后反转5圈）
  * @param  无
  * @retval 无
  * @note   28BYJ-48使用四拍模式，一圈约512步
  *         5圈 = 512 * 5 = 2560步
  */
void Stepper_Motor_OpenDoor(void)
{
    unsigned int steps_per_circle = 512; // 28BYJ-48四拍模式一圈的步数
    unsigned char circles = 5;            // 旋转圈数
    unsigned int total_steps = steps_per_circle * circles; // 总步数：2560步
    
    // 正转5圈
    Stepper_Motor_Rotate(total_steps, 0);
    
    // 中间暂停500ms
    Delay(500);
    
    // 反转5圈
    Stepper_Motor_Rotate(total_steps, 1);
    
    // 完成后确保电机停止
    P1 = P1 & 0xF0; // 清除低4位，关闭所有相
}
