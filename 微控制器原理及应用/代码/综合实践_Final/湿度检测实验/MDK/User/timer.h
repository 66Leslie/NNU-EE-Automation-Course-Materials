#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f4xx.h"
void Tim2_Init(u16 arr,u16 psc);
void TIM2_IRQHandler(void);    
#endif
