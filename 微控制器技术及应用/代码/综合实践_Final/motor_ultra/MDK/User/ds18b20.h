#include "stm32f4xx.h"
#include "delay.h"

#define DS18B20_CLK     RCC_AHB1Periph_GPIOD
#define DS18B20_PIN     GPIO_Pin_7               
#define DS18B20_PORT		GPIOD

u8 DS18B20_Init(void);
float DS18B20_Get_Temp(void);
