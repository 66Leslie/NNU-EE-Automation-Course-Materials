/* 头文件 ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include "ww_spi.h"
#include "spi_LCD.h"
#include "ds18b20.h"
#include "usart.h"
/* define定义 ------------------------------------------------------------------*/
#define EN1_Port  GPIOC
#define EN1_Pin   GPIO_Pin_7  //PWM输出引脚
#define IN1_Port  GPIOC
#define IN1_Pin   GPIO_Pin_6
#define IN2_Port  GPIOC
#define IN2_Pin   GPIO_Pin_5
#define POP_Port  GPIOA
#define POP_Pin   GPIO_Pin_1
#define KEY_Port  GPIOC
#define KEY0_Pin  GPIO_Pin_0  //加速按键
#define KEY1_Pin  GPIO_Pin_1  //减速按键
/* 函数声明 ----------------------------------------------------------------*/
void MOTOR_Init(void);
void TIM_Configuration(u16 Prescaler,u16 Period);
void PWM_Init(void);
void ENCODER_Init(void);
void KEY_Init(void);
void KEY_Proc(void);
void LCD_Dispaly(void);
void rx_proc(void);
void tx_proc(void);
void pid_init(void);
uint16_t pid_proc(int32_t speed);
/* 变量声明 ----------------------------------------------------------------*/
int32_t speed;
int32_t rv,v;
uint32_t count;
uint16_t PWM_Value = 0;
char display_buf[20];
uint16_t i,j;
uint8_t flag;
uint8_t key_value;
float tem;
typedef struct
{
	int32_t t_speed;
	float kp;
	float ki;
	float kd;
	float ek;
	float l_ek;
	float inte;
	float pwm_out;
}PID;
PID pid;
/* 主函数 ----------------------------------------------------------------*/
int main(void)
{ 
	delay_init();//延时函数时钟初始化
	KEY_Init();
	GPIO_Configuration_LCD();
	SPI_Configuration();
	LCD_init();
	TIM_Configuration(SystemCoreClock/1000000-1,1000-1);//1kHz
	MOTOR_Init();
	ENCODER_Init();
	LCD_Dispaly();
	DS18B20_Init();
	uart_init(115200);
	pid_init();
	
	while(1)
	{
		KEY_Proc();
		tem = DS18B20_Get_Temp();
		LCD_Dispaly();
		rx_proc();
	}	
	
}



void MOTOR_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE); 						 
	
	/*IN1引脚初始化*/ 
	GPIO_InitStructure.GPIO_Pin =  IN1_Pin;									//GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(IN1_Port, &GPIO_InitStructure);
	
	/*IN2引脚初始化*/ 
	GPIO_InitStructure.GPIO_Pin =  IN2_Pin;									//GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(IN2_Port, &GPIO_InitStructure);
	
	/*EN1引脚初始化*/ 
	GPIO_InitStructure.GPIO_Pin =  EN1_Pin;									//GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(EN1_Port, &GPIO_InitStructure);
	
	//IN1置1，IN2置0
	GPIO_SetBits(IN1_Port, IN1_Pin);
	GPIO_ResetBits(IN2_Port, IN2_Pin);
	
	
	PWM_Init();
}

//使用PC7作为PWM输出引脚，PC7复用为TIM3的通道2
void PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure; 

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure; 
	
		//时钟使能
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);		
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

		//配置IO口为复用功能-定时器通道
		GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
		GPIO_Init(GPIOC, &GPIO_InitStructure);
	
		//复用功能配置
		GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_TIM3); 

		//定时器配置
		TIM_TimeBaseStructure.TIM_Period=1000-1;   					//自动重装载值
		TIM_TimeBaseStructure.TIM_Prescaler=168-1;     			//定时器分频
		TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
		TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
		TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    //PWM1 Mode configuration: Channel1 
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//使能输出
    TIM_OCInitStructure.TIM_Pulse = 0;	    //占空比初始化
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    //TIM3CH2配置
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);

		//使能定时器
		TIM_Cmd(TIM3, ENABLE);   
		

}

//PA1为脉冲输入口，复用为TIM5CH2脉冲计数
void ENCODER_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//	TIM_ICInitTypeDef TIM_ICInitStructure;

	// 时钟使能
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	// 复用功能配置
	GPIO_PinAFConfig(POP_Port, GPIO_PinSource1, GPIO_AF_TIM5);

	// 配置IO口为复用功能-定时器通道
	GPIO_InitStructure.GPIO_Pin = POP_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	   // 复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	   // 推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;	   // 上拉
	GPIO_Init(POP_Port, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;//使能tim1   
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //设置抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//设置响应优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);

	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	
	TIM_TIxExternalClockConfig(TIM5, TIM_TIxExternalCLK1Source_TI2, TIM_ICPolarity_Falling, 15);
	
	TIM5->CNT = 0;

	TIM_Cmd(TIM5, ENABLE);
	
}

void TIM5_IRQHandler()
{
	if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
}


//TIM1初始化，定时读取脉冲计数值
void TIM_Configuration(u16 Prescaler,u16 Period)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	//使能TIM1时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);

	TIM_InitStructure.TIM_Prescaler = Prescaler;//定时器分频
	TIM_InitStructure.TIM_Period = Period;//自动重装载值
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数模式
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStructure.TIM_RepetitionCounter = 0; //高级定时器特有RCR寄存器
	TIM_TimeBaseInit(TIM1,&TIM_InitStructure);//初始化TIM1

	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);//允许定时器1更新中断
		//中断通道使能
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;//使能tim1   
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //设置抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//设置响应优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	

	TIM_Cmd(TIM1,ENABLE);//使能定时器1
}



//定时器中断回调
void TIM1_UP_TIM10_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM1,TIM_IT_Update))//更新中断
	{
		
		if(flag > 9)
		{
		flag++;
		if(flag == 20 && GPIO_ReadInputDataBit(KEY_Port,KEY0_Pin) == 0)
		{
			key_value = 1;
			flag = 0;
		}
		
		
		if(flag == 20 && GPIO_ReadInputDataBit(KEY_Port,KEY1_Pin) == 0)
		{
			key_value = 2;
			flag = 0;
		}
	}
	i++;
	j++;
	if(i % 20 == 0)
	{
		speed = TIM5->CNT * 60 * 50 / 4;
		PWM_Value = pid_proc(speed);
		TIM_SetCompare2(TIM3, PWM_Value);
		v += speed;
		TIM5->CNT = 0;
		if(i == 200)
		{
			rv = v / 10;
			i = 0;
			v = 0;
		}
	}
	if(j == 1000)
	{
		tx_proc();
		j = 0;
	}
	
		TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
	}

}

//按键初始化
void KEY_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC , ENABLE); 	//使能GPIO时钟					 

	/* KEY引脚初始化*/
	GPIO_InitStructure.GPIO_Pin =  KEY0_Pin | KEY1_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 						//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(KEY_Port, &GPIO_InitStructure);							  //初始化GPIO
	
	/* POP引脚初始化*/
	GPIO_InitStructure.GPIO_Pin =  POP_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 						//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(POP_Port, &GPIO_InitStructure);							  //初始化GPIO
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE); //使能SYSCFG时钟
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC,EXTI_PinSource0);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC,EXTI_PinSource1);
	//SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,EXTI_PinSource6);
 
	EXTI_InitStructure.EXTI_Line=EXTI_Line0 | EXTI_Line1;//中断线0,1
	EXTI_InitStructure.EXTI_LineCmd=ENABLE;//中断使能
	EXTI_InitStructure.EXTI_Mode=EXTI_Mode_Interrupt;//模式中断
	EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Falling;//下降沿
	EXTI_Init(&EXTI_InitStructure);//初始化外部中断
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;//使能EXTI0   
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //设置抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//设置响应优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;//使能EXTI   
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //设置抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//设置响应优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
}

void KEY_Proc(void)
{
	if(key_value == 1)
		{
			if(pid.t_speed <= 4000)
				pid.t_speed += 500;
			else
				pid.t_speed = 4500;
	  }
		
	if(key_value == 2)
		{
			if(pid.t_speed >= 500)
				pid.t_speed -= 500;
			else
				pid.t_speed = 0;
	  }
		
		key_value = 0;
}

//KEY0按下PWM值+100
void EXTI0_IRQHandler(void)  //外部中断线0中断服务函数
{
	flag = 10;
	EXTI_ClearITPendingBit(EXTI_Line0);//消除中断线0上的中断标志位
}

//KEY1按下PWM值-100
void EXTI1_IRQHandler(void)//中断线1的外部中断服务函数
{
	flag = 10;
	EXTI_ClearITPendingBit(EXTI_Line1);//消除中断线1上的中断标志位
}

void pid_init(void)
{
	pid.kp = 1.0;
	pid.ki = 0.5;
	pid.kd = 0.3;
}
	
uint16_t pid_proc(int32_t speed)
{
	pid.ek = pid.t_speed - speed;
	pid.inte += pid.ek;
	pid.pwm_out = pid.kp * pid.ek + pid.ki * pid.inte + pid.kd * (pid.ek - pid.l_ek);
	pid.l_ek = pid.ek;
	return pid.pwm_out;
}
	

//LCD1602显示
void LCD_Dispaly(void)
{
	sprintf(display_buf, "speed=%.4d", v);
	LCD_prints(0,0,display_buf); 
	sprintf(display_buf, "V=%.4d", PWM_Value);
	LCD_prints(0,2,display_buf);
	sprintf(display_buf, "tem=%.2f", tem);
	LCD_prints(7,2,display_buf); 
}

void rx_proc(void)
{
	u8 len;
	u8 i,j;
	u16 k,m;
	if(USART_RX_STA&0x8000)
		{
			len = USART_RX_STA&0X3FFF;
			for(i = 0; i < len; i++)
			{
				k = (USART_RX_BUF[len-1-i] - '0');
				for(j = 0; j < i; j++)
				  k = k*10;
				m = m + k;
			}
			if(m >4500)
				m = 4500;
			pid.t_speed = m;
			USART_RX_STA = 0;
			
		}
	
}

void tx_proc(void)
{
		printf("当前转速为%.4d",speed);
		printf("当前温度为%.2f",tem);
		printf("\r\n");
}
