#include "stm32f4xx.h"
#include "timer.h"

volatile double RH = 0;
volatile double Freq = 0;

// 初始化定时器2，用于频率检测（1s闸门时间）
void Tim2_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  	//TIM2时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//使能PORTA时钟	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; // PA1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; // 复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; // 上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); 
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource1,GPIO_AF_TIM2); // PA1复用为TIM2
	
	// 定时器TIM2初始化
	// 目标：1秒更新一次中断
	// APB1时钟为42MHz，TIM2时钟为84MHz
	// 84000000 / 8400 = 10000 Hz (计数频率)
	// 10000 / 10000 = 1 Hz (更新频率)
	TIM_TimeBaseStructure.TIM_Period = 10000 - 1; 
	TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1; 
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
 
	// 输入捕获配置
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	// 上升沿捕获
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; // 映射到TI2
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;	 // 8分频，每8个脉冲触发一次捕获中断
	TIM_ICInitStructure.TIM_ICFilter = 0x0F; // 滤波
	TIM_ICInit(TIM2,&TIM_ICInitStructure);	
	
	// 使能中断
	TIM_ITConfig(TIM2,TIM_IT_CC2|TIM_IT_Update,ENABLE);

	// 中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 

	TIM_Cmd(TIM2, ENABLE);  // 使能TIM2				 
}

// 定时器2中断服务程序
void TIM2_IRQHandler(void)
{
	static uint32_t count = 0; // 脉冲计数
	
	if(TIM_GetITStatus(TIM2,TIM_IT_CC2) != RESET) // 捕获中断
	{
		TIM_ClearITPendingBit(TIM2,TIM_IT_CC2);
		count++; // 每8个脉冲进一次中断
	}
	
	if(TIM_GetITStatus(TIM2,TIM_IT_Update) != RESET) // 更新中断（1秒一次）
	{
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
		
		// 计算频率
		// count是捕获中断次数，每次中断代表8个脉冲
		Freq = count * 8.0;

		// RH = (Freq - 60000) / 1200 + 50
		if(Freq < 60000)
		{
			RH = 50.0;
		}
		else if(Freq > 120000)
		{
			RH = 100.0;
		}
		else
		{
			RH = (Freq - 60000.0) / 1200.0 + 50.0;
		}
		
		// 清零计数，准备下一秒测量
		count = 0;
	}
}













