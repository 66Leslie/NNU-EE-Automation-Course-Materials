#ifndef __KEY_H
#define __KEY_H	 
#include "stm32f10x.h"	 

#define KEY1  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)//读取按键0
#define KEY2  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)//读取按键1


void KEY_Init(void);//IO初始化				    
#endif
