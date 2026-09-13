
#ifndef _spi_LCD_H
#define _spi_LCD_H
#include "stm32f4xx.h"
#include "ww_spi.h"
#define LCD_RS_1 (GPIOB->ODR |= 1<<0) 		//PA.0--(LCD)RS
#define LCD_RS_0 (GPIOB->ODR &= ~(1<<0))
#define LCD_RW_1 (GPIOB->ODR |= 1<<1)		//PA.1--(LCD)RW
#define LCD_RW_0 (GPIOB->ODR &= ~(1<<1))
#define LCD_EN_1 (GPIOB->ODR |= 1<<2)		//PA.2--(LCD)EN
#define LCD_EN_0 (GPIOB->ODR &= ~(1<<2))

#define  LCD_LDL()    (GPIOA->ODR &= ~(1<<4))
#define  LCD_LDH()    (GPIOA->ODR |= 1<<4)

void GPIO_Configuration_LCD(void);//LCD的控制IO初始化
void LCD_init(void);			//LCD初始化
void LCD_clr(void);				//LCD清屏
void LCD_cmd(uint8_t cmd);			//写指令
void LCD_dat(uint8_t dat);			//写数据
void LCD_pos(uint8_t x,uint8_t y);		//显示位置
void LCD_printc(uint8_t x,uint8_t y,char c);	//定位显示字符
void LCD_prints(uint8_t x,uint8_t y,char *s);	//定位显示字符串







#endif 

