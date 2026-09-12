#ifndef __USART_H
#define __USART_H
#include "stm32f4xx.h"
#include "stdint.h"
#include "stdio.h"

#define USART_REC_LEN 200
extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	

void uart_init(u32 bound);
void USART1_IRQHandler(void) ;               	//串口1中断服务程序
#endif


