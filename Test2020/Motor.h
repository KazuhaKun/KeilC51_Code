#ifndef __MOTOR_H__
#define __MOTOR_H__

void Motor_Init(void);
void Motor_SetSpeed(unsigned char Speed);

// 步进电机控制函数
void Stepper_Motor_Rotate(unsigned int steps, unsigned char direction);
void Stepper_Motor_OpenDoor(void);

extern unsigned char Counter;
extern unsigned char Compare;
extern unsigned char SysTick_1s;
extern unsigned char SysTick_100ms;

#endif
