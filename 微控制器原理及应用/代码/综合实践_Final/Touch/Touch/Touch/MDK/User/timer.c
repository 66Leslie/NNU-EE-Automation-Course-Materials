#include "stm32f4xx.h"
#include "timer.h"

volatile double RH = 0;
volatile double Freq = 0;

void Tim2_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  	//TIM2时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//使能PORTA时钟	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA1
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource1,GPIO_AF_TIM2); //PA1复用位定时器2
	
	//定时器TIM2初始化
	// 1s gate: 84MHz / 8400 = 10kHz, 10kHz / 10000 = 1Hz
	TIM_TimeBaseStructure.TIM_Period = 10000 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
 
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	//上升沿捕获
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //映射到TI2上
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;	 //配置输入分频,每8个沿捕获一次 
	TIM_ICInitStructure.TIM_ICFilter = 0x0F; // 滤波
	TIM_ICInit(TIM2,&TIM_ICInitStructure);	
	
	//使能捕获中断
	TIM_ITConfig(TIM2,TIM_IT_CC2 | TIM_IT_Update,ENABLE);

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM2, ENABLE);  //使能TIM2				 
}

//定时器2中断服务程序
void TIM2_IRQHandler(void)   //TIM2中断
{
	static uint32_t count = 0;
	
	if(TIM_GetITStatus(TIM2,TIM_IT_CC2) != RESET)// capture
	{
		TIM_ClearITPendingBit(TIM2,TIM_IT_CC2);
		count++;
	}
	
	if(TIM_GetITStatus(TIM2,TIM_IT_Update) != RESET) // 1s update
	{
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	
		// count is capture interrupts, each represents 8 pulses
		Freq = count * 8.0;
	
		if(Freq < 60000.0)
		{
			RH = 50.0;
		}
		else if(Freq > 120000.0)
		{
			RH = 100.0;
		}
		else
		{
			RH = (Freq - 60000.0) / 1200.0 + 50.0;
		}
	
		count = 0;
	}
}













