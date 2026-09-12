/* 头文件 ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "delay.h"
#include "spi_LCD.h" // 包含LCD驱动
#include "ww_spi.h"  // 包含SPI配置
#include <stdio.h>
#include <string.h>

/* 变量定义 ----------------------------------------------------------------*/
// 定义时间结构变量
volatile u8 timer_min = 0; // 分
volatile u8 timer_sec = 0; // 秒
volatile u8 timer_ms10 = 0; // 10毫秒位 (0-99)
volatile u8 is_running = 0; // 运行状态标志: 0-暂停, 1-运行

/* 函数声明 ----------------------------------------------------------------*/
void GPIO_Configuration(void);      // LED GPIO配置
void Key_Configuration(void);       // 按键 GPIO配置 (PD1, PD2)
void TIM_Configuration(u16 Prescaler,u16 Period); // 定时器配置

/* 主函数 ----------------------------------------------------------------*/
int main(void)
{ 
    char lcd_buffer[16]; // 存放LCD显示字符串
    u8 key1_stat = 1, key2_stat = 1; // 按键状态记录

    // 1. 系统初始化
    delay_init();       // 延时函数初始化
    GPIO_Configuration(); // LED初始化 (保留原有的LED作为指示灯)
    Key_Configuration();  // 按键初始化 (PD1, PD2)
    
    // 2. LCD初始化
    // 注意：根据你的spi_LCD库，需要先初始化SPI，再初始化LCD
    SPI_Configuration(); // 初始化SPI接口 (来自ww_spi.c)
    GPIO_Configuration_LCD(); // 初始化LCD控制引脚 (来自spi_LCD.c)
    LCD_init();          // LCD屏幕初始化
    LCD_clr();           // 清屏

    // 3. 定时器初始化
    // 目标: 10ms中断一次 (0.01秒)
    // SystemCoreClock=168MHz. Prescaler=1680-1 -> 100kHz. Period=1000-1 -> 100Hz(10ms)
    TIM_Configuration(1680-1, 1000-1); 

    // 初始显示
    LCD_prints(0, 0, "Stopwatch:");
    LCD_prints(0, 1, "00:00:00");

    while (1)
    {
        // --- 按键扫描逻辑 (PD1: 启动/暂停) ---
        if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_1) == 0) // 检测到按键按下
        {
            delay_ms(20); // 消抖
            if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_1) == 0)
            {
                is_running = !is_running; // 切换运行状态
                
                // 状态指示灯 (LED开启表示运行，关闭表示暂停)
                if(is_running) GPIO_ResetBits(GPIOC, GPIO_Pin_0); 
                else GPIO_SetBits(GPIOC, GPIO_Pin_0);

                // 等待按键松开，防止连触
                while(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_1) == 0);
            }
        }

        // --- 按键扫描逻辑 (PD2: 清零) ---
        if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == 0)
        {
            delay_ms(20);
            if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == 0)
            {
                is_running = 0; // 强制暂停
                timer_min = 0;
                timer_sec = 0;
                timer_ms10 = 0;
                
                GPIO_SetBits(GPIOC, GPIO_Pin_0); // 关闭指示灯
                
                // 立即更新屏幕显示清零
                LCD_prints(0, 1, "00:00:00");

                while(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == 0);
            }
        }

        // --- 屏幕刷新逻辑 ---
        // 只有在计时运行状态下才不断刷新，或者你可以加一个刷新标志位
        // 注意：LCD刷新比较慢，不需要每次循环都刷，每100ms刷一次视觉效果较好
        if(is_running)
        {
            // 格式化字符串: 分:秒:毫秒(两位)
            sprintf(lcd_buffer, "%02d:%02d:%02d", timer_min, timer_sec, timer_ms10);
            LCD_prints(0, 1, lcd_buffer); 
            // 适当延时，防止刷新太快导致LCD闪烁或占用总线过高
            delay_ms(50); 
        }
    }
}

/*******************************************************************************
TIM1中断函数 (负责计时)
*******************************************************************************/
void TIM1_UP_TIM10_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM1, TIM_IT_Update)) // 检查更新中断
    {
        if(is_running == 1) // 如果处于运行状态
        {
            timer_ms10++; // 增加10ms计数
            
            if(timer_ms10 >= 100) // 100 * 10ms = 1秒
            {
                timer_ms10 = 0;
                timer_sec++;
                
                if(timer_sec >= 60) // 60秒 = 1分
                {
                    timer_sec = 0;
                    timer_min++;
                    
                    if(timer_min >= 99) // 防止溢出，最大99分
                        timer_min = 0;
                }
            }
        }
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update); // 清除中断标志
    }
}

/*******************************************************************************
按键 GPIO初始化 (PD1, PD2)
*******************************************************************************/
void Key_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOD时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    
    // 配置PD1和PD2为输入模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;       // 输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // 上拉 (默认高电平，按下低电平)
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/*******************************************************************************
LED GPIO初始化 (保留原有的LED配置)
*******************************************************************************/
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);                         
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | 
                                  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 默认关闭所有LED (高电平灭，假设是共阳极，如果是共阴则反之，此处参考原代码推测)
    GPIO_SetBits(GPIOC, GPIO_Pin_All);
}

/*******************************************************************************
TIM1初始化
*******************************************************************************/
void TIM_Configuration(u16 Prescaler, u16 Period)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_InitStructure.TIM_Prescaler = Prescaler;
    TIM_InitStructure.TIM_Period = Period;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_InitStructure);

    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 优先级稍微提高
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);    

    TIM_Cmd(TIM1, ENABLE);
}