/* 头文件 ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include "ww_spi.h"
#include "spi_LCD.h"
#include "timer.h"
/* define定义 ------------------------------------------------------------------*/

/* 函数声明 ----------------------------------------------------------------*/

/* 全局变量 ----------------------------------------------------------------*/
extern u16 RH;

/* 主函数 ----------------------------------------------------------------*/
int main(void)
{ 
	char lcd_dat[16];//液晶显示变量
	delay_init();//延时函数时钟初始化
	GPIO_Configuration_LCD();//gpio初始化
	SPI_Configuration();//spi初始化
	LCD_init();//lcd初始化    
	Tim2_Init(10000-1,SystemCoreClock/2/10000-1);
	LCD_prints(0,0,"    HS1101     ");     
	while(1)
	{
		sprintf(lcd_dat,"Hum=%03d%%    ",RH);//变量转换字符串
		LCD_prints(0,1,lcd_dat);
		delay_ms(1000);	
	}	
}

