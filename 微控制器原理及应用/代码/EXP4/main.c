#include <reg52.h>

//========================================================================
// 宏定义和硬件引脚定义
//========================================================================
#define SEGMENT_PORT P0
sbit LSA = P2^0;
sbit LSB = P2^1;

// *** 修正点: 逻辑已反转 (NPN 驱动) ***
// 1 = ON (打开三极管)
// 0 = OFF (关闭三极管)
sbit BUZZER = P3^7; // 蜂鸣器 (已移至 P3.7)

//... (共阳极段码表... 保持不变) ...
unsigned char code segment_table[] = {
    0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90
};
//... (所有全局变量... 保持不变) ...
volatile unsigned char countdown_seconds = 5;
volatile unsigned char display_tens = 6;
volatile unsigned char display_ones = 0;
volatile unsigned int ms_counter = 0; 
volatile bit second_flag = 0; 
unsigned char scan_digit = 0; 

//... (函数声明... 保持不变) ...
void init_timer0(void);
void handle_countdown_logic(void);

//========================================================================
// 主函数 (后台)
//========================================================================
void main(void) {
    // *** 修正点: 0 = OFF ***
    BUZZER = 0; // 默认关闭蜂鸣器 (P3.7 输出低电平)
    
    init_timer0(); 
    EA = 1;        

    while (1) {
        if (second_flag == 1) {
            second_flag = 0; 
            handle_countdown_logic();
        }
    }
}

//========================================================================
// 后台逻辑处理函数
//========================================================================
void handle_countdown_logic(void) {
    if (countdown_seconds > 0) {
        countdown_seconds--;        
        EA = 0; 
        display_tens = countdown_seconds / 10;
        display_ones = countdown_seconds % 10;
        EA = 1; 
    } 
    else {
        // *** 修正点: 1 = ON ***
        BUZZER = 1; // 蜂鸣器报警 (P3.7 输出高电平)
        TR0 = 0;    // 停止定时器T0
    }
}

//... (Timer0 初始化 和 ISR... 保持不变) ...
// (继续使用 5ms, 0xEC78)
void init_timer0(void) {
    TMOD = 0x01; 
    TH0 = 0xEC; 
    TL0 = 0x78; 
    ET0 = 1; 
    TR0 = 1; 
}

void timer0_isr() interrupt 1 {
    TH0 = 0xEC;
    TL0 = 0x78;

    ms_counter += 5;
    if (ms_counter >= 1000) {
        ms_counter = 0; 
        second_flag = 1;
    }

    LSA = 0; 
    LSB = 0; 
    SEGMENT_PORT = 0xFF; 
    
    scan_digit = !scan_digit; 
    if (scan_digit == 0) {
        SEGMENT_PORT = segment_table[display_tens];
        LSA = 1;
    } else {
        SEGMENT_PORT = segment_table[display_ones];
        LSB = 1;
    }
}