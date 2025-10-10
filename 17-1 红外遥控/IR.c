#include <REGX52.H>
#include "Timer0.h"
#include "Int0.h"

// NEC协议红外驱动相关变量
unsigned int IR_Time;           // 记录定时器计数值，用于判断脉冲宽度
unsigned char IR_State;         // 红外接收状态机当前状态

unsigned char IR_Data[4];       // 存储接收到的4字节数据（地址、地址反码、命令、命令反码）
unsigned char IR_pData;         // 当前接收的bit位序号（0~31）

unsigned char IR_DataFlag;      // 数据接收完成标志位
unsigned char IR_RepeatFlag;    // 重复码接收标志位
unsigned char IR_Address;       // 解码得到的地址码
unsigned char IR_Command;       // 解码得到的命令码

// 红外驱动初始化，配置定时器和外部中断
void IR_Init(void)
{
	Timer0_Init();   // 初始化定时器0，用于测量脉冲宽度
	Int0_Init();     // 初始化外部中断0，接收红外信号
}

// 获取数据接收完成标志，读取后自动清零
unsigned char IR_GetDataFlag(void)
{
	if(IR_DataFlag)
	{
		IR_DataFlag = 0;
		return 1;
	}
	return 0;
}

// 获取重复码标志，读取后自动清零
unsigned char IR_GetRepeatFlag(void)
{
	if(IR_RepeatFlag)
	{
		IR_RepeatFlag = 0;
		return 1;
	}
	return 0;
}

// 获取解码得到的地址码
unsigned char IR_GetAddress(void)
{
	return IR_Address;
}

// 获取解码得到的命令码
unsigned char IR_GetCommand(void)
{
	return IR_Command;
}

// 外部中断0服务函数，NEC协议红外信号解码主流程
void Int0_Routine() interrupt 0
{
	switch(IR_State)
	{
		case 0: // 等待引导码
		{
			Timer0_SetCounter(0);   // 计数器清零
			Timer0_Run(1);          // 启动定时器
			IR_State = 1;           // 切换到下一个状态，等待引导码结束
			break;
		}
		case 1: // 判断引导码/重复码
		{
			IR_Time = Timer0_GetCounter(); // 获取高电平持续时间
			Timer0_SetCounter(0);          // 计数器清零
			// NEC协议引导码：9ms低+4.5ms高，约13.5ms
			if(IR_Time > 12442-500 & IR_Time < 12442+500) // 13.5ms
			{
				IR_State = 2; // 进入数据接收状态
			}
			// NEC协议重复码：9ms低+2.25ms高，约11.25ms
			else if(IR_Time > 10368-500 & IR_Time < 10368+500) // 11.25ms
			{
				IR_RepeatFlag = 1; // 标记为重复码
				Timer0_Run(0);     // 停止定时器
				IR_State = 0;      // 回到初始状态
			}
			else
			{
				IR_State = 1;      // 异常脉冲，继续等待
			}            
			break;
		}
		case 2: // 数据位接收与解码
		{
			IR_Time = Timer0_GetCounter(); // 获取脉冲宽度
			Timer0_SetCounter(0);          // 计数器清零
			// 逻辑0：560us高电平
			if(IR_Time > 1032-500 & IR_Time < 1032+500) // 约560us
			{
				IR_Data[IR_pData/8] &=~(0x01<<(IR_pData%8)); // 当前位为0
				IR_pData++;

			}
			// 逻辑1：1.69ms高电平
			else if(IR_Time > 2074-500 & IR_Time < 2074+500) // 约1.69ms
			{
				IR_Data[IR_pData/8] |=(0x01<<(IR_pData%8)); // 当前位为1
				IR_pData++;
			}
			else
			{
				IR_pData = 0; // 脉冲异常，重新接收
				IR_State = 1;
			}
			// 32位数据接收完毕
			if(IR_pData >= 32)
			{
				IR_pData = 0;
				// 校验地址与命令反码
				if((IR_Data[0] == ~IR_Data[1]) && (IR_Data[2] == ~IR_Data[3]))
				{
					IR_Address = IR_Data[0];   // 提取地址码
					IR_Command = IR_Data[2];   // 提取命令码
					IR_DataFlag = 1;           // 标记数据接收完成
				}
				Timer0_Run(0); // 停止定时器
				IR_State = 0;  // 回到初始状态
			}
			break;
		}
		// ...可扩展其他状态...
	}
    
}
