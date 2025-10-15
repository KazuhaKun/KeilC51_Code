#include <REGX52.H>
#include "LCD1602.h"
#include "delay.h"

// 定义LED连接的端口，假设LED连接到P2口的低5位
#define LED_PORT P2

void main()
{
    unsigned char count = 0;        // 计数器，范围0-31
    bit direction = 0;              // 方向标志，0表示递增，1表示递减
    
    LCD_Init();                     // 初始化LCD1602
    LCD_ShowString(1,1,"Counter:");  // 显示标题
    
    while(1)
    {
        // 更新LED显示（D4-D0对应P2.4-P2.0）
        // 注意：LED低电平点亮，所以需要取反
        LED_PORT = ~count;
        
        // 在LCD上显示当前的十进制数字
        LCD_ShowNum(2,1,count,2);   // 在第2行第1列显示2位数字
        
        // 延时1秒
        Delay(1000);
        
        // 根据方向更新计数器
        if(direction == 0)          // 递增模式
        {
            if(count < 31)
            {
                count++;            // 计数加1
            }
            else
            {
                direction = 1;      // 到达31，切换为递减模式
                count--;            // 开始递减
            }
        }
        else                        // 递减模式
        {
            if(count > 0)
            {
                count--;            // 计数减1
            }
            else
            {
                direction = 0;      // 到达0，切换为递增模式
                count++;            // 开始递增
            }
        }
    }
}