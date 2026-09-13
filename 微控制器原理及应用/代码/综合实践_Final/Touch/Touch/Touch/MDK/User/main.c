/* 头文件 ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include "LCD.h"
#include "TouchPanel.h"
#include "PWM.h"
#include "ds18b20.h"
#include "timer.h"

/* 函数声明 ----------------------------------------------------------------*/
void TPkeys(void);
void TIM_Configuration(void);
void Fan_Init(void);
void Fan_Stop(void);
void Fan_Forward(u8 duty);
void Fan_Reverse(u8 duty);
void ADC_Speed_Init(void);
u16 ADC_Speed_Read(void);
void SpeedMode_Update(void);
void System_Init(void);
u8 Key_read(void);
void Display(void);
void Display_HomeInit(void);
void temp_RD(void);
void LEDWarning(void);
void Warning_Init(void);

/* 变量声明 ----------------------------------------------------------------*/
u8 WarningFlag, OldWarningFlag;
u8 duty;
char dutystr[3], tempStr[4], humStr[4];
u8 turnFlag, runFlag, handFlag; 
u8 speedMode; 
u8 Key_Old, ScreenDly;
u16 KeyDly, KeyStop, tempDly, WarningLEDDly, adcDly, uims;
u16 temp_Value;
extern volatile double RH;





void System_Init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	TIM_Configuration();
	delay_init();					//延时函数时钟初始化
	TP_Init(); 
	LCD_Initializtion();			//LCD引脚初始化
	TouchPanel_Calibrate();			//校验LCD触摸点
	Fan_Init();
	Tim2_Init();
	ADC_Speed_Init();
	Display_HomeInit();
	DS18B20_Init();
	Warning_Init();
}

void Warning_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 	//使能GPIO时钟					 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;             //GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);									//初始化GPIO
	
	GPIO_Write(GPIOC, (GPIO_ReadOutputData(GPIOC)|0xff00));
}
/*******************************************************************************
Fan初始化
*******************************************************************************/
void Fan_Init(void)
{
	PWM_Init();
	
	/*EN引脚初始化*/ 
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//使能GPIO时钟					 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;             //GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
 	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);									//初始化GPIO
	
	Fan_Stop();
}

void Fan_Stop(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_10, Bit_RESET);
	TIM_SetCompare1(TIM4, 0);
	TIM_SetCompare1(TIM3, 0);
}

void Fan_Forward(u8 duty)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_10, Bit_SET);
	TIM_SetCompare1(TIM4, duty);
	TIM_SetCompare1(TIM3, 0);
}

void Fan_Reverse(u8 duty)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_10, Bit_SET);
	TIM_SetCompare1(TIM4, 0);
	TIM_SetCompare1(TIM3, duty);
}

void ADC_Speed_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_Cmd(ADC1, ENABLE);
}

u16 ADC_Speed_Read(void)
{
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_144Cycles);
	ADC_SoftwareStartConv(ADC1);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {}
	return ADC_GetConversionValue(ADC1);
}

void SpeedMode_Update(void)
{
	u16 adcValue;
	u32 tmp;
	u8 newDuty;

	if(!speedMode) return;
	if(adcDly) return;

	adcValue = ADC_Speed_Read();
	tmp = (u32)adcValue * 85;
	newDuty = 10 + (u8)(tmp / 4095);

	if(newDuty > 95) newDuty = 95;
	if(newDuty < 10) newDuty = 10;
	duty = newDuty;

	if(runFlag)
	{
		if(turnFlag) Fan_Forward(duty);
		else Fan_Reverse(duty);
	}
}




/* 主函数 ----------------------------------------------------------------*/
int main(void)
{ 
	System_Init();
	
	WarningFlag = 0;
	duty = 50;
	turnFlag = 1;
	runFlag = 0;
	handFlag = 0;
	speedMode = 0;
	
	uims = 0;
	KeyDly = 0;
	KeyStop = 0;
	tempDly = 0;
	WarningLEDDly = 0;
	adcDly = 0;
	
	while(1)
	{	
		temp_RD();
		TPkeys();
		SpeedMode_Update();
		Display();
		LEDWarning();
	}	

}



u8 Key_read(void)
{
	getDisplayPoint(&display, Read_Ads7846(), &matrix) ;//读取触摸屏触点，转换成LCD坐标
	//if(display.x<120 && display.y>120) return 1;
	if(display.x>120 && display.x<220 && display.y>120 && display.y<180) return 2;
	if(display.x>120 && display.x<220 && display.y>180 && display.y<240) return 3;
	if(display.x>220 && display.x<320 && display.y>120 && display.y<150) return 8;
	if(display.x>220 && display.x<320 && display.y>150 && display.y<180) return 4;
	if(display.x>220 && display.x<320 && display.y>180 && display.y<240) return 5;
	if(display.x<120 && display.x>0 && display.y>120 && display.y<180) return 6;
	if(display.x<120 && display.x>0 && display.y>180 && display.y<240) return 7;
	
	return 0;
}

void Display(void)
{
	u8 WarningFlag_Down, WarningFlag_Down2;
	
	if(ScreenDly) return;
	WarningFlag_Down = WarningFlag & (WarningFlag ^ OldWarningFlag);
	WarningFlag_Down2 = WarningFlag | !(WarningFlag ^ OldWarningFlag);
	OldWarningFlag = WarningFlag;

	
	
	if(WarningFlag == 0)       //转速面板
	{
		sprintf(dutystr,"%d",duty);
		GUI_Char(55,100,(uint8_t *)dutystr,0xffff,Black);
		sprintf(tempStr,"%4.1f",temp_Value/10.0);
		GUI_Char(215,100,(uint8_t *)tempStr,0xffff,Black);
		sprintf(humStr,"%d",(int)RH);
		GUI_Char(215,75,(uint8_t *)humStr,0xffff,Black);
		GUI_Chinese(265,130,"调速",speedMode ? Green : 0xffff,Black);

	}
	if(WarningFlag_Down == 1)  //报警面板
	{
		LCD_Clear(Yellow);
		GUI_Chinese(160-15,120-10,"警告",Red,Yellow);
		GUI_Chinese(280,220,"确认",Black,Yellow);
	}
	if(WarningFlag_Down2 == 0)
	{
		Display_HomeInit();
	}
}

void Display_HomeInit(void)
{
	LCD_Clear(Black);

	GUI_Chinese(70,10,"基于",0xffff,Black);
	GUI_Char(105,10,"HS1101",0xffff,Black);
	GUI_Chinese(155,10,"的湿度测量仪",0xffff,Black);
	
	GUI_Char(30,35,"21230933",0xffff,Black);
	GUI_Chinese(100,35,"赵学文",0xffff,Black);			
	GUI_Char(170,35,"21230920",0xffff,Black);
	GUI_Chinese(240,35,"陈铭佳",0xffff,Black);
	GUI_Char(112,55,"21230931",0xffff,Black);
	GUI_Chinese(176,55,"金可",0xffff,Black);
	
	GUI_Chinese(25,160,"启动",0xffff,Black);	
	GUI_Chinese(25,200,"停止",0xffff,Black);
	GUI_Chinese(145,160,"正转",0xffff,Black);
	GUI_Chinese(145,200,"反转",0xffff,Black);
	GUI_Chinese(265,130,"调速",0xffff,Black);
	GUI_Chinese(265,160,"加速",0xffff,Black);
	GUI_Chinese(265,200,"减速",0xffff,Black);
	
	GUI_Chinese(10,75,"转向",0xffff,Black);
	GUI_Char(40,75,":",0xffff,Black);
	GUI_Chinese(55,75,"正转",0xffff,Black);
	
	GUI_Chinese(10,100,"转速",0xffff,Black);
	GUI_Char(40,100,":",0xffff,Black);
	
	GUI_Chinese(170,75,"湿度",0xffff,Black);
	GUI_Char(200,75,":",0xffff,Black);
	GUI_Char(215,75,"70",0xffff,Black);
	GUI_Char(235,75,"%",0xffff,Black);
	
	GUI_Chinese(170,100,"温度",0xffff,Black);
	GUI_Char(200,100,":",0xffff,Black);
	GUI_Char(250,100,"*C",0xffff,Black);
}

void TPkeys(void)
{
	u8 Key_Value, Key_Down;
	
	if(KeyDly) return;
	if(KeyStop>1) return;
	
	Key_Value = Key_read();
	Key_Down = Key_Value & (Key_Value ^ Key_Old);
	Key_Old = Key_Value;
	
	if (Key_Down) KeyStop = 250;
	
	if(WarningFlag == 1)  // comfirm
	{
		if(Key_Down == 5)       //Warning 面板 confirm back to menu
		{
			WarningFlag = 0;
			handFlag = 1;
			Display_HomeInit();
		}
	}
	else if(WarningFlag == 0)   //menu
	{
		if(Key_Down == 2)
		{
			if(!turnFlag) 
			{
				Fan_Stop();
				runFlag = 0;
			}
			turnFlag = 1;
			GUI_Chinese(55,75,"正转",0xffff,Black);
		}
		else if(Key_Down == 3)
		{
			if(turnFlag) 
			{
				Fan_Stop();
				runFlag = 0;
			}
			turnFlag = 0;
			GUI_Chinese(55,75,"反转",0xffff,Black);
		}
		else if(Key_Down == 8)
		{
			speedMode = !speedMode;
			adcDly = 0;
		}
		else if(Key_Down == 4)
		{
			if(speedMode) return;
			if(duty < 95)duty += 5;
			if(runFlag)
			{
				if(turnFlag)Fan_Forward(duty);
				else Fan_Reverse(duty);
			}
		}
		else if(Key_Down == 5)
		{
			if(speedMode) return;
			if(duty > 10)duty -= 5;
			if(runFlag)
			{
				if(turnFlag)Fan_Forward(duty);
				else Fan_Reverse(duty);
			}
		}
		else if(Key_Down == 6)
		{
			runFlag = 1;
			if(turnFlag)Fan_Forward(duty);
				else Fan_Reverse(duty);
		}
		else if(Key_Down == 7)
		{
			runFlag = 0;
			Fan_Stop();
		}
	}
}

void temp_RD(void)
{
	if(tempDly) return;
	temp_Value = DS18B20_Get_Temp();
}

void LEDWarning(void)
{
	u16 low8,high8;
	
	if(WarningLEDDly) return;
	
	if (RH > 60)
		Fan_Forward(90);
	
	if (RH > 60 && handFlag == 0) 
		WarningFlag = 1;
	else 
		WarningFlag = 0;
	
	if(WarningFlag && handFlag == 0)
	{
		low8 = GPIO_ReadOutputData(GPIOC) & 0x00ff;
		high8 = ~GPIO_ReadOutputData(GPIOC) & 0xff00;
		GPIO_Write(GPIOC, low8 | high8);
	}
	else if(handFlag)
		GPIO_Write(GPIOC, (GPIO_ReadOutputData(GPIOC)&0x00ff));
	else
		GPIO_Write(GPIOC, (GPIO_ReadOutputData(GPIOC)|0xff00));
}	

/*******************************************************************************
TIM1初始化,定时器1定时1s
*******************************************************************************/
void TIM_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	//使能TIM1时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);

	TIM_InitStructure.TIM_Prescaler = SystemCoreClock/10000-1;//定时器分频
	TIM_InitStructure.TIM_Period = 10-1;//自动重装载值
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

/*******************************************************************************
TIM1中断函数
*******************************************************************************/
void TIM1_UP_TIM10_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM1,TIM_IT_Update))//更新中断
	{
		if(++uims==6000) 
		{
			handFlag = 0;
			uims = 0;
		}
		
		if(++KeyDly==20) KeyDly = 0;
		
		if(KeyStop>0) KeyStop--;
		
		if(++ScreenDly==10) ScreenDly = 0;
		
		if(++tempDly==500) tempDly = 0;
		
		if(++WarningLEDDly==500) WarningLEDDly = 0;
		if(++adcDly==20) adcDly = 0;
		
		TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
	}
}
