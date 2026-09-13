#include <reg52.h>

#define SEG_TENS    P0
#define SEG_ONES    P2

sbit LED_YELLOW  = P1^2;
sbit LED_GREEN   = P1^3;
sbit LED_RED     = P1^4;

unsigned char code segment_table[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99,
    0x92, 0x82, 0xF8, 0x80, 0x90
};

volatile unsigned char countdown_seconds = 59;
volatile unsigned char timer_ticks = 0;

void Init_Timer0(void);
void Update_Display(void);

void main(void)
{
    LED_GREEN  = 1;
    LED_YELLOW = 0;
    LED_RED    = 0;

    Update_Display();
    Init_Timer0();

    while (1) {
    }
}

void Init_Timer0(void)
{
    TMOD = 0x01;
    TH0  = 0x3C;
    TL0  = 0xB0;

    EA  = 1;
    ET0 = 1;
    TR0 = 1;
}

void Update_Display(void)
{
    if (countdown_seconds > 99) {
        countdown_seconds = 99;
    }

    SEG_TENS = segment_table[countdown_seconds / 10];
    SEG_ONES = segment_table[countdown_seconds % 10];
}

void Timer0_ISR() interrupt 1
{
    TH0 = 0x3C;
    TL0 = 0xB0;

    timer_ticks++;

    if (timer_ticks >= 20) {
        timer_ticks = 0;

        if (countdown_seconds > 0) {
            countdown_seconds--;
            Update_Display();

            if (countdown_seconds > 3) {
                LED_GREEN = 1;
                LED_YELLOW = 0;
                LED_RED = 0;
            } else if (countdown_seconds > 0) {
                LED_GREEN = 0;
                LED_YELLOW = 1;
                LED_RED = 0;
            } else {
                LED_GREEN = 0;
                LED_YELLOW = 0;
                LED_RED = 1;
                TR0 = 0;
            }
        }
    }
}
/* 以下是思考题代码
#include <reg52.h>

//========================================================================
// 硬件引脚定义 (基于 image_c4453c.png)
//========================================================================

// 数码管段选端口 (P0 控制 a-dp)
#define SEG_PORT P0

// 数码管位选引脚 (P2 控制 1-2-3-4)
// 共阳极数码管：位选引脚给高电平(1)时选中该位
sbit DIGIT_1 = P2^0; // 第1位 (左) - 分钟十位
sbit DIGIT_2 = P2^1; // 第2位 - 分钟个位
sbit DIGIT_3 = P2^2; // 第3位 - 秒钟十位
sbit DIGIT_4 = P2^3; // 第4位 (右) - 秒钟个位

// LED 灯引脚 (高电平点亮)
sbit LED_YELLOW = P1^2;
sbit LED_GREEN  = P1^3;
sbit LED_RED    = P1^4;
sbit LED_BLUE   = P1^5; // 新增蓝灯

//========================================================================
// 全局变量与常量
//========================================================================

// 共阳极段码表 (0-9, 不带小数点)
unsigned char code segment_table[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 
    0x92, 0x82, 0xF8, 0x80, 0x90
};

// 倒计时总秒数
// 设定为 125秒 (2分05秒)，以便测试蓝灯逻辑
volatile unsigned int total_seconds = 125; 

// 显示缓冲区 (存储4个位置要显示的数字 0-9)
volatile unsigned char display_buff[4] = {0, 0, 0, 0};

// 扫描控制变量
volatile unsigned char scan_index = 0; // 当前扫描到第几位
volatile unsigned int ms_counter = 0;  // 毫秒计数

//========================================================================
// 函数声明
//========================================================================
void Init_Timer0(void);
void Update_Display_Buffer(void);
void Traffic_Light_Logic(void);

//========================================================================
// 主函数
//========================================================================
void main(void) {
    // 1. 初始化
    Update_Display_Buffer(); // 先计算一次初始显示
    Traffic_Light_Logic();   // 先设置一次灯的状态
    Init_Timer0();           // 启动定时器
    
    // 2. 主循环
    while(1) {
        // 主循环可以处理非实时逻辑，或者空转
        // 所有的显示刷新和计时都在中断里完成
    }
}

//========================================================================
// 定时器初始化 (2ms @ 12MHz)
// 扫描频率：2ms一位，4位循环约8ms，刷新率 > 100Hz，不闪烁
//========================================================================
void Init_Timer0(void) {
    TMOD = 0x01; // 模式1 (16位)
    // 2ms = 2000us
    // 初值 = 65536 - 2000 = 63536 = 0xF830
    TH0 = 0xF8;
    TL0 = 0x30;
    
    EA = 1;  // 开总中断
    ET0 = 1; // 开定时器中断
    TR0 = 1; // 启动
}

//========================================================================
// 逻辑处理：计算分/秒 并填入显示缓冲
//========================================================================
void Update_Display_Buffer(void) {
    unsigned char minutes;
    unsigned char seconds;
    
    // 将总秒数转换为 分:秒
    minutes = total_seconds / 60;
    seconds = total_seconds % 60;
    
    // 填入缓冲区
    display_buff[0] = minutes / 10; // 分钟十位
    display_buff[1] = minutes % 10; // 分钟个位
    display_buff[2] = seconds / 10; // 秒钟十位
    display_buff[3] = seconds % 10; // 秒钟个位
}

//========================================================================
// 逻辑处理：交通灯控制
//========================================================================
void Traffic_Light_Logic(void) {
    // 先关闭所有灯
    LED_BLUE = 0; LED_GREEN = 0; LED_YELLOW = 0; LED_RED = 0;

    if (total_seconds > 120) {
        // 超过2分钟 ( > 120s ) -> 蓝灯
        LED_BLUE = 1; 
    }
    else if (total_seconds > 3) {
        // 3秒 到 2分钟 ( 3 < t <= 120 ) -> 绿灯
        LED_GREEN = 1;
    }
    else if (total_seconds > 0) {
        // 1秒 到 3秒 ( 0 < t <= 3 ) -> 黄灯
        LED_YELLOW = 1;
    }
    else {
        // 0秒 -> 红灯
        LED_RED = 1;
        TR0 = 0; // 停止计时
    }
}

//========================================================================
// 定时器中断 (每2ms触发一次)
// 负责：1. 数码管动态扫描  2. 秒计时
//========================================================================
void Timer0_ISR() interrupt 1 {
    // 重装初值 2ms
    TH0 = 0xF8;
    TL0 = 0x30;
    
    //-----------------------------------
    // 1. 数码管动态扫描逻辑
    //-----------------------------------
    // (A) 消隐：先关闭位选，防止鬼影
    P2 &= 0xF0; // P2.0-P2.3 置0
    
    // (B) 送段码
    SEG_PORT = segment_table[display_buff[scan_index]];
    
    // (C) 打开对应的位选 (P2.0 ~ P2.3)
    switch(scan_index) {
        case 0: DIGIT_1 = 1; break;
        case 1: DIGIT_2 = 1; break;
        case 2: DIGIT_3 = 1; break;
        case 3: DIGIT_4 = 1; break;
    }
    
    // 切换到下一位，如果超过3则回到0
    scan_index++;
    if (scan_index >= 4) {
        scan_index = 0;
    }
    
    //-----------------------------------
    // 2. 秒计时逻辑
    //-----------------------------------
    ms_counter++;
    // 2ms * 500 = 1000ms = 1秒
    if (ms_counter >= 500) {
        ms_counter = 0;
        
        if (total_seconds > 0) {
            total_seconds--;
            Update_Display_Buffer(); // 更新显示数字
            Traffic_Light_Logic();   // 更新灯光状态
        }
    }
}
/*