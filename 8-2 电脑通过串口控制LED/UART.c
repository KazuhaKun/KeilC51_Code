#include <REGX52.H>

/*
    *@brief 串口初始化 4800bps@11.0592MHz
    *@param 无
    *@retval 无
*/

void UART_Init()        //4800bps@11.0592MHz
{
/*
    SM0 = 0;
    SM1 = 1;
    SM2 = 0;
    REN = 0;
    TB8 = 0;
    RB8 = 0;
    TI = 0;
    RI = 0;
*/
	PCON |= 0x80;		//使能波特率倍速位SMOD
    SCON =0x50;
    TMOD &= 0x0F;	//清除低四位
	TMOD |= 0x20;	//设置定时器模式1,16位定时器模式
	TL1 = 0xF4;		//设定定时初值
	TH1 = 0xF4;		//设定定时器重装值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
    EA = 1;         //开总中断
    ES = 1;         //开串口中断
}

/*
    *@brief 串口发送一个字节数据
    *@param Byte:待发送的一个字节数据
    *@retval 无
*/

void UART_SendByte(unsigned char Byte)
{
    SBUF = Byte;
    while(TI == 0);
    TI = 0;
}

/*
void UART_Routine() interrupt 4
{
    if(RI == 1)
    {

        RI = 0;
    }
}
*/