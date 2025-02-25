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
	// PCON |= 0x80;		//使能波特率倍速位SMOD
    // SCON =0x40;
    // TMOD &= 0x0F;	//清除低四位
	// TMOD |= 0x20;	//设置定时器模式1,16位定时器模式
	// TL1 = 0xF4;		//设定定时初值
	// TH1 = 0xF4;		//设定定时器重装值
	// ET1 = 0;		//禁止定时器1中断
	// TR1 = 1;		//启动定时器1


	// PCON |= 0x80;		//使能波特率倍速位SMOD
	// SCON = 0x50;		//8位数据,可变波特率
	// TMOD &= 0x0F;		//清除定时器1模式位
	// TMOD |= 0x20;		//设定定时器1为8位自动重装方式
	// TL1 = 0xF4;		//设定定时初值
	// TH1 = 0xF4;		//设定定时器重装值
	// ET1 = 0;		//禁止定时器1中断
	// TR1 = 1;		//启动定时器1

	PCON &= 0x7F;		//波特率不倍速
	SCON = 0x50;		//8位数据,可变波特率
	TMOD &= 0x0F;		//清除定时器1模式位
	TMOD |= 0x20;		//设定定时器1为8位自动重装方式
	TL1 = 0xFA;		//设定定时初值
	TH1 = 0xFA;		//设定定时器重装值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
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
