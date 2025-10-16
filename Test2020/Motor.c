#include <REGX52.H>
#include "Timer1.h"
#include "main.h"
#include "Delay.h"

// // 引脚定义 - 直流电机（原有的PWM控制）
// sbit Motor=P1^0;

// 步进电机引脚定义 - 28BYJ-48 (使用P1口的低4位)
// 连接到ULN2003D的IN1-IN4
sbit Stepper_A = P1^0;  // 对应电机A相，连接ULN2003D的IN1
sbit Stepper_B = P1^1;  // 对应电机B相，连接ULN2003D的IN2
sbit Stepper_C = P1^2;  // 对应电机C相，连接ULN2003D的IN3
sbit Stepper_D = P1^3;  // 对应电机D相，连接ULN2003D的IN4

unsigned char Counter,Compare;

// /**
//   * @brief  电机初始化
//   * @param  无
//   * @retval 无
//   */
void Motor_Init(void)
{
	Timer1_Init();
}

// /**
//   * @brief  电机设置速度
//   * @param  Speed 要设置的速度，范围0~100
//   * @retval 无
//   */
// void Motor_SetSpeed(unsigned char Speed)
// {
// 	Compare=Speed;
// }

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

	// if(Counter<Compare)	//计数值小于比较值
	// {
	// 	Motor=1;		//输出1
	// }
	// else				//计数值大于比较值
	// {
	// 	Motor=0;		//输出0
	// }
}

/*========== 步进电机控制函数 ==========*/

// 八拍驱动时序数组（半步模式，高精度）
// 对应 P1.3 P1.2 P1.1 P1.0 -> D C B A
// code unsigned char phases[8] = {
//     0x01, // 0001B (A)
//     0x03, // 0011B (A, B)
//     0x02, // 0010B (B)
//     0x06, // 0110B (B, C)
//     0x04, // 0100B (C)
//     0x0C, // 1100B (C, D)
//     0x08, // 1000B (D)
//     0x09  // 1001B (D, A)
// };

unsigned char  xdata phases[8] = {   // 八拍驱动相位表（每相依次通电）
    0x01, 0x03, 0x02, 0x06,   // A -> AB -> B -> BC
    0x04, 0x0C, 0x08, 0x09    // C -> CD -> D -> DA
};

/**
  * @brief  步进电机开门动作（正转10圈后反转10圈）
  * @param  无
  * @retval 无
  * @note   28BYJ-48使用八拍模式，一圈约2048步
  *         10圈 = 2048 * 10 = 20480步
  */
void Stepper_Motor_OpenDoor(void)
{
    unsigned int s;
    unsigned char step;
    
    // 正转 10 圈 (之前是反转逻辑，现在用于正转)
    for(s = 0; s < 10 * 2048; s++)
    {
        step = s % 8;
        P1 = (P1 & 0xF0) | phases[7 - step]; // 使用反转时序
        Delay(1);
    }

    // 反转 10 圈 (之前是正转逻辑，现在用于反转)
    for(s = 0; s < 10 * 2048; s++)
    {
        step = s % 8;
        P1 = (P1 & 0xF0) | phases[step]; // 使用正转时序
        Delay(1);
    }

    // 停止：关闭所有线圈
    P1 &= 0xF0;
}
