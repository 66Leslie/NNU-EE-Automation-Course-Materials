#include "gpio.h"

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE , ENABLE);  						 

	// 按键初始化
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 						//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// 电机引脚
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//LED初始化	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	//LED灯初始化
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4 ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	
}





