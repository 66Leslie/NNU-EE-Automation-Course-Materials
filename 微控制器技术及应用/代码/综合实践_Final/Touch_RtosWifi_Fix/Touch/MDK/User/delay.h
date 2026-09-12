#ifndef __DELAY_H
#define __DELAY_H 			   
#include "stm32f4xx.h"

void delay_init(void);
void delay_us(u32 us);
void delay_ms(u16 ms);

/* 1ms tick counter (SysTick interrupt) */
uint32_t delay_get_ms(void);

#endif





























