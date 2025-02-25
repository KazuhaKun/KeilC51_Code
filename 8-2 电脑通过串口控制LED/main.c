// #include <REGX52.H>
// #include "delay.h"
// #include "UART.h"

// void main()
// {
//     UART_Init();
//     UART_SendByte(0x41);
//     while(1)
//     {

//     }
// }

// void UART_Routine() interrupt 4
// {
//     if(RI == 1)
//     {
//         P2 = ~SBUF;
//         UART_SendByte(SBUF);
//         RI = 0;
//     }
// }
#include <REGX52.H>
#include "Delay.h" 
#include "UART.h"

void main()
{
    UART_Init();
	Delay(1);
    while(1)
    {
    
    }
}

void UART_Routine() interrupt 4
{
    P2 = 0x00;
}
