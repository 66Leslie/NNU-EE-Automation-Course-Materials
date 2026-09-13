#ifndef __DS18B20_H
#define	__DS18B20_H
 
#include "stm32f4xx.h"
#include "delay.h"
 
#define HIGH  1
#define LOW   0
 
#define DS18B20_CLK     RCC_AHB1Periph_GPIOB
#define DS18B20_PIN     GPIO_Pin_12                  
#define DS18B20_PORT		GPIOB 
 
//带参宏，可以像内联函数一样使用,输出高电平或低电平
#define DS18B20_DATA_OUT(a)	if (a)	\
					GPIO_SetBits(GPIOB,GPIO_Pin_12);\
					else		\
					GPIO_ResetBits(GPIOB,GPIO_Pin_12)
 //读取引脚的电平
#define  DS18B20_DATA_IN()	   GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)
 
typedef struct
{

	u8  temp_int;	 	//温度的整数部分
	u8  temp_deci;	 	//温度的小数部分
	u8  check_sum;	 	//校验和
		                 
}DS18B20_Data_TypeDef;
 
u8 DS18B20_Init(void);
float DS18B20_Get_Temp(void);
#endif /* __DS18B20_H */
