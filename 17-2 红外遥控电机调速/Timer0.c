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
	TR0 = 0;          //不启动定时器
	TL0 = 0x18;		//设置定时初值低字节
	TH0 = 0xFC;		//设置定时初值高字节
}

void Timer0_SetCounter(unsigned int Value)
{
	TH0 = Value/256;
	TL0 = Value%256;
}

unsigned int Timer0_GetCounter(void)
{
	return (TH0<<8)|TL0;
}

void Timer0_Run(unsigned char Flag)
{
	TR0 = Flag;
}
