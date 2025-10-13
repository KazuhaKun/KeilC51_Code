#include <REGX52.H>
#include "Timer1.h"
#include "main.h"

// 引脚定义
sbit Motor=P1^0;

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
