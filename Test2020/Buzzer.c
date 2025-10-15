#include <REGX52.H>
#include <intrins.h>

sbit Buzzer = P2^5;

void Buzzer_Delay500us()		//@11.0592MHz
{
	unsigned char i;

	_nop_();
	i = 227;
	while (--i);
}

void Buzzer_Time(unsigned int ms)
{
    unsigned int i;
    for(i = 0; i < ms*2; i++)
    {
        Buzzer =!Buzzer;
        Buzzer_Delay500us();
    }
}

/**
  * @brief  蜂鸣器报警3秒
  * @param  无
  * @retval 无
  * @note   用于密码错误时的报警提示
  *         频率：约1kHz（500us周期翻转）
  *         持续时间：3000ms
  */
void Buzzer_Alarm_3s(void)
{
    // 报警3秒，频率约1kHz
    Buzzer_Time(3000);
    
    // 确保蜂鸣器停止（输出低电平）
    Buzzer = 1;
}
