#include <REGX52.H>

/*
    *@brief 定时器0初始化
    *@param 无
    *@retval 无
*/
void Timer1_Init()
{
	TMOD &= 0x0F;	//清除低四位
	TMOD |= 0x10;	//设置定时器模式1,16位定时器模式
	TF1 = 0;          //清除TF1标志
	TR1 = 1;          //启动定时器
	TL1 = 0xA4;		//设置定时初值低字节
	TH1 = 0xFF;		//设置定时初值高字节
	ET1 = 1;          //启用定时器1中断
	EA = 1;           //启用总中断
	PT1 = 0;          //设置定时器1中断优先级为低
}

/* @template
void Timer1_Routine() interrupt 3
{
	static unsigned int T1Count;
	TL1 = 0x66; 
	TH1 = 0xFC;
	T1Count++;
	if(T1Count > 1000) //1000ms
	{
		T1Count = 0;
		//要运行的命令
	}
}
*/