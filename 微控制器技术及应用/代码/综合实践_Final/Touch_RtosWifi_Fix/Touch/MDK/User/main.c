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
#include "eeprom.h"
#include "esp8266.h"


/* 硬件资源配置 --------------------------------------------------------------*/
/* 工作指示灯：PC0（高电平点亮，低电平熄灭） */
#define WORK_LED_GPIO        GPIOC
#define WORK_LED_PIN         GPIO_Pin_0
#define WORK_LED_RCC         RCC_AHB1Periph_GPIOC

/* 启动按键：PB7（上拉输入，按下接地为低电平） */
#define START_KEY_GPIO       GPIOC
#define START_KEY_PIN        GPIO_Pin_7
#define START_KEY_RCC        RCC_AHB1Periph_GPIOC
#define START_KEY_IS_PRESSED() (GPIO_ReadInputDataBit(START_KEY_GPIO, START_KEY_PIN) == Bit_RESET)

/* 电机测速脉冲输入：PB0（上拉输入，外部脉冲） */
#define SPEED_IN_GPIO         GPIOB
#define SPEED_IN_PIN          GPIO_Pin_0
#define SPEED_IN_RCC          RCC_AHB1Periph_GPIOB
#define SPEED_IN_PORT_SOURCE  EXTI_PortSourceGPIOB
#define SPEED_IN_PIN_SOURCE   EXTI_PinSource0
#define SPEED_IN_EXTI_LINE    EXTI_Line0
#define SPEED_IN_EXTI_IRQn    EXTI0_IRQn
#define SPEED_PULSE_PER_REV   20      /* 每转脉冲数，按模块实际输出修改 */
#define SPEED_CALC_INTERVAL_MS 1000

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
void SpeedPulse_Init(void);
void SpeedPulse_Update(void);
void System_Init(void);
u8 Key_read(void);
void Display(void);
void Display_HomeInit(void);
void temp_RD(void);
void LEDWarning(void);
void Warning_Init(void);
void WorkLED_Init(void);
void WorkLED_On(void);
void WorkLED_Off(void);
void StartKey_Init(void);
u8   StartKey_Scan(void);
void Display_StandbyInit(void);
void Display_StandbyUpdateTime(void);
void Display_HistoryInit(void);
void History_DrawPlot(void);
void History_Storage_Init(void);
void History_Storage_Append(u16 temp_x10);
u8   History_Storage_ReadAll(u16 *buf, u8 maxCount);

static void LCD_FillRect_ByLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
/* 变量声明 ----------------------------------------------------------------*/
u8 WarningFlag, OldWarningFlag;
u8 duty;
char speedStr[6], tempStr[4], humStr[4];
u8 turnFlag, runFlag, handFlag; 
u8 speedMode; 
u8 Key_Old, ScreenDly;
u16 KeyDly, KeyStop, tempDly, WarningLEDDly, adcDly, uims;
u16 temp_Value;
volatile uint32_t SpeedPulseCnt;
u16 motor_rpm;
extern volatile double RH;

/* 历史温度保存参数（可按需要修改） */
#define HIST_MAX_SAMPLES        60      /* 屏幕最多绘制60个点 */
#define HIST_SAVE_INTERVAL_MS   5000    /* 每5秒保存一次温度到EEPROM */







/* 上电待机/按键启动：系统状态与按键消抖计时 */
u8 SysRunFlag;      /* 0=待机页面，1=运行主界面 */
u16 StartKeyDly;

/* 历史数据：界面与存储 */
u8 uiPage;                 /* 0=主界面，1=历史数据界面 */
volatile u8  HistSaveReq;      /* 1=需要保存一次历史温度 */
volatile u16 HistSaveDlyMs;    /* 历史温度保存定时，单位ms */
u8 EepromIoErr;             /* EEPROM读写错误标志：1=通信失败/未写入 */
   /* 启动按键消抖计时，单位ms（由1ms定时中断递减） */
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
	SpeedPulse_Init();
	DS18B20_Init();
	Warning_Init();

	/* EEPROM(24C64) 初始化：用于历史数据存储 */
	EEPROM24C64_Init();
	History_Storage_Init();

	/* 额外外设：启动按键 + 工作指示灯 */
	WorkLED_Init();
	StartKey_Init();

	/* 上电默认待机：电机停止、指示灯灭、显示等待页面 */
	SysRunFlag = 0;
	uiPage = 0;
	HistSaveReq = 0;
	HistSaveDlyMs = HIST_SAVE_INTERVAL_MS;
    EepromIoErr = 0;          /* 清零EEPROM错误标志 */
	Fan_Stop();
	WorkLED_Off();
	Display_StandbyInit();

	/* WiFi模块初始化：ESP8266（AT固件） */
	ESP8266_Init();
	/* WiFi初始化完成后，刷新待机页显示IP */
	Display_StandbyInit();
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

void SpeedPulse_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(SPEED_IN_RCC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitStructure.GPIO_Pin = SPEED_IN_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(SPEED_IN_GPIO, &GPIO_InitStructure);

	SYSCFG_EXTILineConfig(SPEED_IN_PORT_SOURCE, SPEED_IN_PIN_SOURCE);

	EXTI_InitStructure.EXTI_Line = SPEED_IN_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = SPEED_IN_EXTI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	SpeedPulseCnt = 0;
	motor_rpm = 0;
}

void SpeedPulse_Update(void)
{
	static uint32_t last_ms = 0;
	uint32_t now = delay_get_ms();

	if((uint32_t)(now - last_ms) < SPEED_CALC_INTERVAL_MS) return;
	{
		uint32_t elapsed = now - last_ms;
		uint32_t cnt;
		uint32_t freq;
		uint32_t rpm;

		last_ms = now;

		__disable_irq();
		cnt = SpeedPulseCnt;
		SpeedPulseCnt = 0;
		__enable_irq();

		if(elapsed == 0) return;
		freq = (cnt * 1000U) / elapsed;
		rpm = (freq * 60U) / SPEED_PULSE_PER_REV;
		if(rpm > 9999U) rpm = 9999U;
		motor_rpm = (u16)rpm;
	}
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

	SysRunFlag = 0;
	StartKeyDly = 0;
	uiPage = 0;
	HistSaveReq = 0;
	HistSaveDlyMs = HIST_SAVE_INTERVAL_MS;

	while(1)
	{
		ESP8266_SetWorkState(SysRunFlag);
		{
			double rh = RH;
			uint16_t hum_x10;
			if(rh < 0.0) rh = 0.0;
			if(rh > 100.0) rh = 100.0;
			hum_x10 = (uint16_t)(rh * 10.0 + 0.5);
			ESP8266_SetTelemetry((int16_t)temp_Value, hum_x10, duty);
		}
		ESP8266_Task();
		SpeedPulse_Update();
	
		/* 启动按键：实现“待机 <-> 运行”模式切换 */
		if(StartKey_Scan())
		{
			if(SysRunFlag == 0)
			{
				/* 待机 -> 运行 */
				SysRunFlag = 1;
				WorkLED_On();									/* 指示灯亮：系统工作中 */
				Display_HomeInit();							/* 切换到原有主界面 */
				Key_Old = 0;									/* 清除触摸残留 */
				OldWarningFlag = 0;
			}
			else
			{
				/* 运行 -> 待机 */
				SysRunFlag = 0;
				uiPage = 0;
				HistSaveReq = 0;
				HistSaveDlyMs = HIST_SAVE_INTERVAL_MS;
				Fan_Stop();									/* 停止风扇/执行机构 */
				WorkLED_Off();								/* 指示灯灭：系统待机 */
				WarningFlag = 0;							/* 关闭报警状态 */
				OldWarningFlag = 0;
				GPIO_Write(GPIOC, (GPIO_ReadOutputData(GPIOC)|0xff00));	/* 关闭报警灯（PC8~PC15置高） */
				Display_StandbyInit();					/* 切换到待机页面 */
			}
			continue;										/* 切换后本轮不再执行其他逻辑 */
		}

		/* 待机状态：只刷新时间并等待按键 */
		if(SysRunFlag == 0)
		{
			Display_StandbyUpdateTime();
			continue;
		}

		/* 运行状态：保持原有功能不变 */
		temp_RD();

		/* 定时保存温度到EEPROM（工作状态才保存） */
		if(HistSaveReq)
		{
			HistSaveReq = 0;
			History_Storage_Append(temp_Value);
			/* 如果正在历史界面，实时刷新曲线 */
			if(uiPage == 1 && WarningFlag == 0)
				History_DrawPlot();
		}

		TPkeys();
		SpeedMode_Update();
		Display();
		LEDWarning();
	}	

}
u8 Key_read(void)
{
	Coordinate *screen;

	if(TP_INT_IN) return 0;
	screen = Read_Ads7846();
	if(screen == 0) return 0;
	if(getDisplayPoint(&display, screen, &matrix) == DISABLE) return 0;

	/* 报警确认面板：只允许点击“确认”区域返回 */
	if(WarningFlag == 1)
	{
		if(display.x >= 220 && display.x <= 319 && display.y >= 180 && display.y <= 239) return 5; //确认
		return 0;
	}

		/* 历史数据界面：只保留“返回”按钮 */
	if(uiPage == 1)
	{
		/* 右上角返回：x=230~319, y=0~50 */
		if(display.x >= 230 && display.x <= 319 && display.y <= 50) return 10;
		return 0;
	}

/* 主界面：启动/停止、正转/反转、调速模式（旋钮调速开关） */
	if(display.x >= 120 && display.x <= 219 && display.y >= 120 && display.y <= 179) return 2; //正转
	if(display.x >= 120 && display.x <= 219 && display.y >= 180 && display.y <= 239) return 3; //反转

		/* 右侧两块区域：上方=旋钮调速开关，下方=历史数据 */
	if(display.x >= 220 && display.x <= 319 && display.y >= 120 && display.y <= 179) return 8; //旋钮调速开关
	if(display.x >= 220 && display.x <= 319 && display.y >= 180 && display.y <= 239) return 9; //历史数据


	if(display.x <= 119 && display.y >= 120 && display.y <= 179) return 6; //启动
	if(display.x <= 119 && display.y >= 180 && display.y <= 239) return 7; //停止

	return 0;
}
void Display(void)
{
	u8 WarningFlag_Down, WarningFlag_Down2;

	if(ScreenDly) return;
	WarningFlag_Down  = WarningFlag & (WarningFlag ^ OldWarningFlag);
	WarningFlag_Down2 = WarningFlag | !(WarningFlag ^ OldWarningFlag);
	OldWarningFlag = WarningFlag;

	if(WarningFlag == 0)       //主界面/历史界面显示
	{
		if(uiPage == 1) return;  //历史界面时不刷新主界面文字，避免覆盖

		sprintf(speedStr,"%4u",(unsigned int)motor_rpm);
		GUI_Char(55,100,(uint8_t *)speedStr,0xffff,Black);

		sprintf(tempStr,"%4.1f",temp_Value/10.0);
		GUI_Char(215,100,(uint8_t *)tempStr,0xffff,Black);

		sprintf(humStr,"%d",(int)RH);
		GUI_Char(215,75,(uint8_t *)humStr,0xffff,Black);

		/* 调速模式状态提示：只保留“调速模式”按键 + ADC旋钮 */
		GUI_Chinese(238,132,"旋钮调速", speedMode ? Green : Grey, Black);
		GUI_Chinese(248,158, (uint8_t *)(speedMode ? "开启" : "关闭"), speedMode ? Green : Grey, Black);
	}

	if(WarningFlag_Down == 1)  //报警界面
	{
		char warnStr[16];

		LCD_Clear(Black);
		LCD_FillRect_ByLine(0, 0, 319, 36, Red);
		GUI_Chinese(130, 10, (uint8_t *)"警告", White, Red);
		LCD_DrawLine(0, 36, 319, 36, White);

		LCD_FillRect_ByLine(20, 60, 299, 165, Yellow);
		GUI_Chinese(70, 78, (uint8_t *)"湿度过高", Red, Yellow);
		sprintf(warnStr, "RH:%d%%", (int)RH);
		GUI_Char(110, 110, (uint8_t *)warnStr, Black, Yellow);
		GUI_Chinese(52, 138, (uint8_t *)"请确认后返回", Black, Yellow);

		LCD_FillRect_ByLine(220, 180, 319, 239, Green);
		GUI_Chinese(248, 202, (uint8_t *)"确认", Black, Green);
	}
	if(WarningFlag_Down2 == 0)
	{
		/* 报警解除后，回到进入报警前的界面 */
		if(uiPage == 1)
		{
			Display_HistoryInit();
			History_DrawPlot();
		}
		else
		{
			Display_HomeInit();
		}
	}
}
void Display_HomeInit(void)
{
	LCD_Clear(Black);

	/* 标题栏与人员信息 */
	GUI_Chinese(42,10,"智能家居温湿度监控与电机控制终端",0xffff,Black);

	GUI_Char(30,35,"21230933",0xffff,Black);
	GUI_Chinese(100,35,"赵学文",0xffff,Black);
	GUI_Char(170,35,"21230920",0xffff,Black);
	GUI_Chinese(240,35,"陈铭佳",0xffff,Black);
	GUI_Char(112,55,"21230931",0xffff,Black);
	GUI_Chinese(176,55,"金可",0xffff,Black);

	/* 按键标签（删除“加速/减速”，只保留“调速模式”按键） */
	GUI_Chinese(25,160,"启动",0xffff,Black);
	GUI_Chinese(25,200,"停止",0xffff,Black);
	GUI_Chinese(145,160,"正转",0xffff,Black);
	GUI_Chinese(145,200,"反转",0xffff,Black);

	GUI_Chinese(238,132,"旋钮调速",0xffff,Black);
	GUI_Chinese(248,158, (uint8_t *)(speedMode ? "开启" : "关闭"), speedMode ? Green : Grey, Black);
	GUI_Chinese(238,200,"历史数据",0xffff,Black);

	/* 状态显示区 */
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

	/* 画出按钮边框，界面更清晰 */
	/* 启动按钮框：x=0~119, y=120~179 */
	LCD_DrawLine(0,120,119,120,Grey);
	LCD_DrawLine(0,179,119,179,Grey);
	LCD_DrawLine(0,120,0,179,Grey);
	LCD_DrawLine(119,120,119,179,Grey);

	/* 停止按钮框：x=0~119, y=180~239 */
	LCD_DrawLine(0,180,119,180,Grey);
	LCD_DrawLine(0,239,119,239,Grey);
	LCD_DrawLine(0,180,0,239,Grey);
	LCD_DrawLine(119,180,119,239,Grey);

	/* 正转按钮框：x=120~219, y=120~179 */
	LCD_DrawLine(120,120,219,120,Grey);
	LCD_DrawLine(120,179,219,179,Grey);
	LCD_DrawLine(120,120,120,179,Grey);
	LCD_DrawLine(219,120,219,179,Grey);

	/* 反转按钮框：x=120~219, y=180~239 */
	LCD_DrawLine(120,180,219,180,Grey);
	LCD_DrawLine(120,239,219,239,Grey);
	LCD_DrawLine(120,180,120,239,Grey);
	LCD_DrawLine(219,180,219,239,Grey);

		/* 旋钮调速按钮框：x=220~319, y=120~179 */
	LCD_DrawLine(220,120,319,120,Grey);
	LCD_DrawLine(220,179,319,179,Grey);
	LCD_DrawLine(220,120,220,179,Grey);
	LCD_DrawLine(319,120,319,179,Grey);

	/* 历史数据按钮框：x=220~319, y=180~239 */
	LCD_DrawLine(220,180,319,180,Grey);
	LCD_DrawLine(220,239,319,239,Grey);
	LCD_DrawLine(220,180,220,239,Grey);
	LCD_DrawLine(319,180,319,239,Grey);

	/* 分隔线：让主区域更“板式” */
	/* 删除蓝色分割线：按需求不再绘制 */
}


/* 待机页面：上电后显示“等待启动”，不响应触摸按键 */

/* 历史数据界面：绘制坐标系与按钮 */
void Display_HistoryInit(void)
{
    /* 背景与标题 */
    LCD_Clear(Black);
    GUI_Chinese(10,10,"历史数据",White,Black);

    /* 返回按钮（右上角） */
    GUI_Chinese(255,14,"返回",Black,Yellow);
    LCD_DrawLine(230,0,319,0,Yellow);
    LCD_DrawLine(230,50,319,50,Yellow);
    LCD_DrawLine(230,0,230,50,Yellow);
    LCD_DrawLine(319,0,319,50,Yellow);

    /* 坐标系区域：左=30 右=310 上=60 下=210 */
    LCD_DrawLine(30,60,310,60,Grey);
    LCD_DrawLine(30,210,310,210,Grey);
    LCD_DrawLine(30,60,30,210,Grey);
    LCD_DrawLine(310,60,310,210,Grey);

    /* 坐标轴 */
    LCD_DrawLine(30,210,310,210,White);     /* X轴 */
    LCD_DrawLine(30,60,30,210,White);       /* Y轴 */

    /* 轴标题（避免挡住绘图区） */
    GUI_Chinese(40,45,"温度",White,Black);
    GUI_Char(80,45,"(C)",White,Black);
    GUI_Chinese(270,214,"时间",White,Black);
}



/* 用画水平线的方式填充矩形区域（LCD库未提供填充函数时使用）
 * 注意：坐标包含边界，即会绘制 y1~y2 的每一条水平线
 */
static void LCD_FillRect_ByLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    u16 y, t;
    if(x2 < x1){ t = x1; x1 = x2; x2 = t; }
    if(y2 < y1){ t = y1; y1 = y2; y2 = t; }

    for(y = y1; y <= y2; y++)
    {
        LCD_DrawLine(x1, y, x2, y, color);
    }
}

/* 绘图专用直线函数（修复原LCD_DrawLine不支持负斜率导致折线断裂的问题）
 * 说明：原工程LCD_DrawLine会分别对x0/x1、y0/y1做大小交换，导致负斜率线段端点错位。
 * 本函数采用通用Bresenham算法，支持任意方向，适合历史曲线折线绘制。
 */
static void Plot_DrawLine(int x0, int y0, int x1, int y1, u16 color)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while(1)
    {
        LCD_SetPoint((u16)x0, (u16)y0, color);
        if(x0 == x1 && y0 == y1) break;

        {
            int e2 = err << 1;
            if(e2 > -dy) { err -= dy; x0 += sx; }
            if(e2 <  dx) { err += dx; y0 += sy; }
        }
    }
}




/* 读取EEPROM中的历史温度并绘制折线（带纵坐标刻度） */
void History_DrawPlot(void)
{
    u16 buf[HIST_MAX_SAMPLES];
    u8  cnt, i;
    u16 tMin, tMax, tNow;

    /* 为纵坐标预留文字区域，把绘图区右移一些 */
    u16 left   = 60;
    u16 right  = 310;
    u16 top    = 70;
    u16 bottom = 210;

    char s[16];

    /* 若EEPROM通信已经报错，直接提示，避免显示“假数据” */
    if(EepromIoErr)
    {
        /* 不清右上角“返回”按钮区域（0~50） */
        LCD_FillRect_ByLine(0, 55, 319, 239, Black);

        GUI_Chinese(30,90,"EEPROM通信失败",Red,Black);
        GUI_Chinese(30,120,"请检查SDA/SCL接线",Grey,Black);
        GUI_Chinese(30,150,"以及上拉电阻/供电",Grey,Black);
        return;
    }

    /* 读取历史数据 */
    cnt = History_Storage_ReadAll(buf, HIST_MAX_SAMPLES);
    if(cnt < 2)
    {
        LCD_FillRect_ByLine(0, 55, 319, 239, Black);
        GUI_Chinese(30,120,"暂无足够数据",White,Black);
        GUI_Chinese(30,150,"请运行一段时间后再看",Grey,Black);
        return;
    }

    /* 计算Min/Max/Now（单位：0.1℃） */
    tMin = buf[0];
    tMax = buf[0];
    for(i=0;i<cnt;i++)
    {
        if(buf[i] < tMin) tMin = buf[i];
        if(buf[i] > tMax) tMax = buf[i];
    }
    tNow = buf[cnt-1];

    /* 让纵坐标范围“好看”一点：上下各留0.5℃边距，避免贴边 */
    if(tMax == tMin)
    {
        /* 温度几乎恒定时，人工给一个范围，避免除0 */
        if(tMin > 10) tMin -= 10;
        tMax = tMin + 20;
    }
    else
    {
        if(tMin > 5) tMin -= 5;
        tMax += 5;
    }

    /* 清空绘图区与纵坐标区域，避免多次刷新叠加导致“条纹/断线” */
    LCD_FillRect_ByLine(0, 55, 319, 239, Black);

    /* 重新绘制坐标系边框 */
    LCD_DrawLine(left,  top,    right, top,    White);
    LCD_DrawLine(left,  bottom, right, bottom, White);
    LCD_DrawLine(left,  top,    left,  bottom, White);
    LCD_DrawLine(right, top,    right, bottom, White);

    /* 横向网格线：4等分（共5条含上下边界） */
    for(i=0;i<5;i++)
    {
        u16 y = (u16)(bottom - (u32)(bottom-top) * i / 4);
        LCD_DrawLine(left, y, right, y, Grey);

        /* Y轴刻度短线 */
        LCD_DrawLine(left-3, y, left, y, White);

        /* Y轴刻度数值（摄氏度，1位小数） */
        {
            u16 v = (u16)(tMin + (u32)(tMax - tMin) * i / 4);
            sprintf(s, "%d.%d", (int)(v/10), (int)(v%10));
            /* 左侧对齐显示（预留足够空间） */
            GUI_Char(5, (y>8)?(y-8):y, (uint8_t *)s, White, Black);
        }
    }

    /* 坐标轴标题 */
    GUI_Chinese(left-5, 45, "温度", White, Black);
    GUI_Char(left+35,   45, "(C)",  White, Black);
    GUI_Chinese(right-35, bottom+4, "时间", White, Black);

    if(ESP8266_TimeIsValid())
    {
        uint8_t hh, mm, ss;
        uint32_t now_sec, start_sec, total_sec, interval_sec;
        uint32_t day_sec = 24U * 3600U;
        char ts[10];

        ESP8266_GetTime(&hh, &mm, &ss);
        interval_sec = (uint32_t)(HIST_SAVE_INTERVAL_MS / 1000);
        if(interval_sec == 0) interval_sec = 1;
        total_sec = (cnt > 1) ? ((uint32_t)(cnt - 1) * interval_sec) : 0;
        now_sec = (uint32_t)hh * 3600U + (uint32_t)mm * 60U + (uint32_t)ss;
        start_sec = (now_sec + day_sec - (total_sec % day_sec)) % day_sec;

        {
            uint8_t sh = (uint8_t)(start_sec / 3600U);
            uint8_t sm = (uint8_t)((start_sec % 3600U) / 60U);
            snprintf(ts, sizeof(ts), "%02d:%02d", (int)sh, (int)sm);
            GUI_Char(left, bottom + 12, (uint8_t *)ts, White, Black);
        }

        snprintf(ts, sizeof(ts), "%02d:%02d", (int)hh, (int)mm);
        GUI_Char(right - 40, bottom + 12, (uint8_t *)ts, White, Black);
    }

    /* 右上角统计信息：Min/Max/Now（摄氏度） */
    sprintf(s, "Min:%d.%dC", (int)(tMin/10), (int)(tMin%10));
    GUI_Char(170,10,(uint8_t *)s,White,Black);
    sprintf(s, "Max:%d.%dC", (int)(tMax/10), (int)(tMax%10));
    GUI_Char(170,25,(uint8_t *)s,White,Black);
    sprintf(s, "Now:%d.%dC", (int)(tNow/10), (int)(tNow%10));
    GUI_Char(170,40,(uint8_t *)s,White,Black);
    sprintf(s, "N=%d", cnt);
    GUI_Char(265,40,(uint8_t *)s,White,Black);

    /* 画折线（黄色+红点） */
    for(i=0;i<cnt-1;i++)
    {
        u16 x1,x2,y1,y2;
        u32 dx = (right-left-2);
        u32 dy = (bottom-top-2);

        x1 = (u16)(left + 1 + (u32)i * dx / (cnt-1));
        x2 = (u16)(left + 1 + (u32)(i+1) * dx / (cnt-1));

        y1 = (u16)(bottom - 1 - (u32)(buf[i]   - tMin) * dy / (tMax - tMin));
        y2 = (u16)(bottom - 1 - (u32)(buf[i+1] - tMin) * dy / (tMax - tMin));

        /* 边界保护 */
        if(y1 < top+1) y1 = top+1;
        if(y1 > bottom-1) y1 = bottom-1;
        if(y2 < top+1) y2 = top+1;
        if(y2 > bottom-1) y2 = bottom-1;

        Plot_DrawLine(x1,y1,x2,y2,Yellow);
        LCD_SetPoint(x1,y1,Red);
        LCD_SetPoint(x2,y2,Red);
    }
}


/* --------------------------- 历史数据存储（EEPROM） ---------------------------*/
/* EEPROM地址规划：
   0x0000~0x0007 : 元数据（magic + writeIndex + count）
   0x0010开始    : 温度数据（每条2字节，单位0.1℃，循环队列）
*/
#define EEPROM_HIST_META_ADDR   0x0000
#define EEPROM_HIST_DATA_ADDR   0x0010
#define EEPROM_HIST_MAGIC       0xA55A

static void History_WriteMeta(u8 widx, u8 cnt)
{
    u8 meta[4];
    meta[0] = (u8)(EEPROM_HIST_MAGIC >> 8);
    meta[1] = (u8)(EEPROM_HIST_MAGIC & 0xFF);
    meta[2] = widx;
    meta[3] = cnt;

    /* 写入元数据：若失败，置错误标志，不再更新显示数据 */
    if(EEPROM24C64_Write(EEPROM_HIST_META_ADDR, meta, 4))
        EepromIoErr = 1;
}


static void History_ReadMeta(u8 *widx, u8 *cnt, u8 *ok)
{
    u8 meta[4];
    *ok = 0;

    /* 读取元数据：读失败说明EEPROM通信异常 */
    if(EEPROM24C64_Read(EEPROM_HIST_META_ADDR, meta, 4) != 0)
    {
        EepromIoErr = 1;
        return;
    }

    if(((u16)meta[0] << 8 | meta[1]) != EEPROM_HIST_MAGIC) return;

    *widx = meta[2];
    *cnt  = meta[3];

    if(*widx >= HIST_MAX_SAMPLES) *widx = 0;
    if(*cnt  >  HIST_MAX_SAMPLES) *cnt  = HIST_MAX_SAMPLES;
    *ok = 1;
}


void History_Storage_Init(void)
{
	u8 widx, cnt, ok;
	History_ReadMeta(&widx, &cnt, &ok);
	if(!ok)
	{
		/* 首次使用：清空元数据 */
		History_WriteMeta(0,0);
	}
}

/* 追加一条温度数据（单位0.1℃） */
void History_Storage_Append(u16 temp_x10)
{
    u8 widx, cnt, ok;
    u8 data[2];

    History_ReadMeta(&widx, &cnt, &ok);
    if(!ok)
    {
        widx = 0;
        cnt  = 0;
    }

    /* 温度合理性保护：DS18B20常用范围 -55.0~125.0℃
       本项目用的是0.1℃单位的整数，这里做一个宽松判断（0~150.0℃）。
       若数据异常则不保存，避免EEPROM被错误值污染。 */
    if(temp_x10 > 1500)
        return;

    /* 写入温度（2字节，大端） */
    data[0] = (u8)(temp_x10 >> 8);
    data[1] = (u8)(temp_x10 & 0xFF);
    if(EEPROM24C64_Write((u16)(EEPROM_HIST_DATA_ADDR + (u16)widx * 2), data, 2))
    {
        EepromIoErr = 1;
        return; /* 写失败：不更新元数据 */
    }

    /* 更新指针与计数 */
    widx++;
    if(widx >= HIST_MAX_SAMPLES) widx = 0;
    if(cnt < HIST_MAX_SAMPLES) cnt++;

    History_WriteMeta(widx, cnt);
}


/* 读取所有温度数据，按时间先后输出到buf，返回实际条数 */
u8 History_Storage_ReadAll(u16 *buf, u8 maxCount)
{
    u8 widx, cnt, ok;
    u8 start, i;
    u8 data[2];
    u8 valid = 0;

    History_ReadMeta(&widx, &cnt, &ok);
    if(!ok || cnt == 0) return 0;
    if(cnt > maxCount) cnt = maxCount;

    /* 计算最旧数据的下标 */
    start = (u8)((widx + HIST_MAX_SAMPLES - cnt) % HIST_MAX_SAMPLES);

    for(i=0;i<cnt;i++)
    {
        u8 idx = (u8)((start + i) % HIST_MAX_SAMPLES);

        if(EEPROM24C64_Read((u16)(EEPROM_HIST_DATA_ADDR + (u16)idx * 2), data, 2))
        {
            EepromIoErr = 1;
            return 0;
        }

        /* 解析并做一次合理性过滤（0~150.0℃），避免无效数据导致曲线“看不见” */
        {
            u16 v = (u16)((u16)data[0] << 8 | data[1]);
            if(v <= 1500)
                buf[valid++] = v;
        }
    }

    return valid;
}



void Display_StandbyInit(void)
{
	/* 待机页面：上电默认进入此页。使用丰富颜色绘制背景与图案 */
	uint16_t y;
	uint16_t color;
	uint8_t r, g, b;

	/* 1) 天空渐变背景（上深下浅） */
	for(y = 0; y < 240; y++)
	{
		/* 这里用简单线性插值生成RGB渐变 */
		r = (uint8_t)(10  + (y * 20) / 239);
		g = (uint8_t)(40  + (y * 80) / 239);
		b = (uint8_t)(120 + (y * 120) / 239);
		color = (uint16_t)RGB565CONVERT(r, g, b);
		LCD_DrawLine(0, y, 319, y, color);
	}

	/* 2) 地面色带 */
	LCD_DrawLine(0, 200, 319, 200, Green);
	LCD_DrawLine(0, 201, 319, 201, Green);
	LCD_DrawLine(0, 202, 319, 202, Green);

	/* 3) 彩色装饰条（模拟波纹/光带） */
	LCD_DrawLine(0, 170, 319, 170, Cyan);
	LCD_DrawLine(0, 171, 319, 171, Blue2);
	LCD_DrawLine(0, 172, 319, 172, Cyan);

	LCD_DrawLine(0, 185, 319, 185, Yellow);
	LCD_DrawLine(0, 186, 319, 186, Magenta);
	LCD_DrawLine(0, 187, 319, 187, Yellow);

	/* 4) 太阳（用点阵填充圆形，位于右上角） */
	{
		int16_t cx = 270, cy = 55, rr = 22;
		int16_t x, yy;
		for(yy = -rr; yy <= rr; yy++)
		{
			for(x = -rr; x <= rr; x++)
			{
				if((x*x + yy*yy) <= rr*rr)
					LCD_SetPoint((uint16_t)(cx + x), (uint16_t)(cy + yy), Yellow);
			}
		}
		/* 太阳外圈 */
		LCD_DrawLine(270-26, 55, 270+26, 55, White);
		LCD_DrawLine(270, 55-26, 270, 55+26, White);
	}

	/* 5) 标题与提示 */
	GUI_Chinese(96, 86, "系统待机", White, Black);
	GUI_Chinese(64, 120, "等待启动", White, Black);
	GUI_Chinese(48, 150, "按下按键进入工作模式", White, Black);

	/* 6) 小装饰框（提示区域） */
	LCD_DrawLine(30, 78, 289, 78, White);
	LCD_DrawLine(30, 78, 30, 180, White);
	LCD_DrawLine(289, 78, 289, 180, White);
	LCD_DrawLine(30, 180, 289, 180, White);
	/* WiFi IP 显示（连接路由器后由ESP8266获取） */
	GUI_Char(38, 200, (uint8_t*)"WiFi IP:", White, Black);
	GUI_Char(38, 216, (uint8_t*)ESP8266_GetIP(), White, Black);

	GUI_Chinese(38, 188, "时间", White, Black);
	GUI_Char(70, 188, ":", White, Black);
	GUI_Char(90, 188, (uint8_t*)"--:--:--", White, Black);

}

void Display_StandbyUpdateTime(void)
{
	static uint8_t last_sec = 0xFF;
	static uint8_t last_valid = 0;
	uint8_t h = 0, m = 0, s = 0;
	uint8_t valid = ESP8266_TimeIsValid();
	char tstr[12];

	if(!valid)
	{
		if(last_valid == 0) return;
		last_valid = 0;
		GUI_Char(90, 188, (uint8_t*)"--:--:--", White, Black);
		return;
	}

	ESP8266_GetTime(&h, &m, &s);
	if(last_valid && s == last_sec) return;
	last_valid = 1;
	last_sec = s;

	snprintf(tstr, sizeof(tstr), "%02d:%02d:%02d", (int)h, (int)m, (int)s);
	GUI_Char(90, 188, (uint8_t*)tstr, White, Black);
}


/* 工作指示灯初始化（PB0） */
void WorkLED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(WORK_LED_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = WORK_LED_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(WORK_LED_GPIO, &GPIO_InitStructure);

	WorkLED_Off();
}

void WorkLED_On(void)
{
	/* 说明：本项目板载/外接LED为低电平点亮（常见接法：VCC->电阻->LED->IO口） */
	GPIO_WriteBit(WORK_LED_GPIO, WORK_LED_PIN, Bit_RESET);
}

void WorkLED_Off(void)
{
	/* LED熄灭：输出高电平 */
	GPIO_WriteBit(WORK_LED_GPIO, WORK_LED_PIN, Bit_SET);
}

/* 启动按键初始化（PB7，上拉输入，按下为低） */
void StartKey_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(START_KEY_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = START_KEY_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(START_KEY_GPIO, &GPIO_InitStructure);

	StartKeyDly = 0;
}

/* 启动按键扫描：返回1表示检测到一次有效按下（带去抖） */
u8 StartKey_Scan(void)
{
	if(StartKeyDly) return 0;

	if(START_KEY_IS_PRESSED())
	{
		StartKeyDly = 250;	// 250ms 去抖/锁定
		return 1;
	}
	return 0;
}
void TPkeys(void)
{
	u8 Key_Value, Key_Down;

	if(KeyDly) return;
	if(KeyStop > 1) return;

	Key_Value = Key_read();
	Key_Down  = Key_Value & (Key_Value ^ Key_Old);
	Key_Old   = Key_Value;

	if(Key_Down) KeyStop = 250;

	/* 报警确认界面 */
	if(WarningFlag == 1)
	{
		if(Key_Down == 5)       //报警面板：确认返回主界面
		{
			WarningFlag = 0;
			handFlag = 1;
			Display_HomeInit();
		}
	}
	/* 主界面/历史界面 */
	else
	{
		/* 历史数据界面：仅处理返回键 */
		if(uiPage == 1)
		{
			if(Key_Down == 10)
			{
				uiPage = 0;
				Display_HomeInit();
			}
			return;
		}

		if(Key_Down == 2)        //正转
		{
			if(!turnFlag)
			{
				Fan_Stop();
				runFlag = 0;
			}
			turnFlag = 1;
			GUI_Chinese(55,75,"正转",0xffff,Black);
		}
		else if(Key_Down == 3)   //反转
		{
			if(turnFlag)
			{
				Fan_Stop();
				runFlag = 0;
			}
			turnFlag = 0;
			GUI_Chinese(55,75,"反转",0xffff,Black);
		}
		else if(Key_Down == 8)   //调速模式开关（只保留此屏幕调速按键）
		{
			speedMode = !speedMode;
			adcDly = 0;
		}
		else if(Key_Down == 9)   //历史数据
		{
			uiPage = 1;
			Display_HistoryInit();
			History_DrawPlot();
			Key_Old = 0;
		}

		else if(Key_Down == 6)   //启动
		{
			runFlag = 1;
			if(turnFlag) Fan_Forward(duty);
			else Fan_Reverse(duty);
		}
		else if(Key_Down == 7)   //停止
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

		/* 启动按键去抖计时递减 */
		if(StartKeyDly>0) StartKeyDly--;

		/* 历史温度保存定时（仅工作状态有效） */
		if(SysRunFlag)
		{
			if(HistSaveDlyMs>0) HistSaveDlyMs--;
			else
			{
				HistSaveReq = 1;
				HistSaveDlyMs = HIST_SAVE_INTERVAL_MS;
			}
		}
		else
		{
			HistSaveDlyMs = HIST_SAVE_INTERVAL_MS;
			HistSaveReq = 0;
		}
		
		if(++ScreenDly==10) ScreenDly = 0;
		
		if(++tempDly==500) tempDly = 0;
		
		if(++WarningLEDDly==500) WarningLEDDly = 0;
		if(++adcDly==20) adcDly = 0;

		ESP8266_TimeTick1ms();

		TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
	}
}

void EXTI0_IRQHandler(void)
{
	if(EXTI_GetITStatus(SPEED_IN_EXTI_LINE) != RESET)
	{
		SpeedPulseCnt++;
		EXTI_ClearITPendingBit(SPEED_IN_EXTI_LINE);
	}
}
