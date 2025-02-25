#include <REGX52.H>

/*
    *@brief 定时器0初始化
    *@param 无
    *@retval 无
*/
void Timer0_Init()
{
	TMOD &= 0xF0;	//清除低四位
	TMOD |= 0x01;	//设置定时器模式1,16位定时器模式
	TF0 = 0;          //清除TF0标志
	TR0 = 1;          //启动定时器
	TL0 = 0xA4;		//设置定时初值低字节
	TH0 = 0xFF;		//设置定时初值高字节
	ET0 = 1;          //启用定时器0中断
	EA = 1;           //启用总中断
	PT0 = 0;          //设置定时器0中断优先级为低
}

/* @template
void Timer0_Routine() interrupt 1
{
	static unsigned int T0Count;
	TL0 = 0x66; 
	TH0 = 0xFC;
	T0Count++;
	if(T0Count > 1000) //1000ms
	{
		T0Count = 0;
		//要运行的命令
	}
}
*/