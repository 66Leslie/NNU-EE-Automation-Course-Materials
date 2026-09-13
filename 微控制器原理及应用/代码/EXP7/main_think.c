#include <reg52.h>

#define GPIO_DIG P2
#define GPIO_KEY P1

typedef unsigned char u8;
typedef unsigned int u16;

// 查找表：用于修正硬件接线反转的问题
u8 code rev_map[] = 
    {0x00,0x08,0x04,0x0C,0x02,0x0A,0x06,0x0E,0x01,0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F};

// 简单的软件延时，用于按键去抖
void delay(u16 i) {
    while (i--);
}

// 键盘扫描函数 (保持原逻辑不变)
u8 KeyScan() {
    static const u8 row_drive[4] = {0xFE, 0xFD, 0xFB, 0xF7};
    static const u8 col_mask[4] = {0x80, 0x40, 0x20, 0x10};
    u8 row, col;
    
    GPIO_KEY = 0x0f;
    if ((GPIO_KEY & 0x0f) == 0x0f) {
        return 16; // 无按键
    }
    delay(300); // 去抖
    if ((GPIO_KEY & 0x0f) == 0x0f) {
        return 16;
    }
    for (row = 0; row < 4; row++) {
        GPIO_KEY = row_drive[row];
        if ((GPIO_KEY & 0xf0) != 0xf0) {
            for (col = 0; col < 4; col++) {
                if ((GPIO_KEY & col_mask[col]) == 0) {
                    GPIO_KEY = 0x0f; // 释放总线
                    return row * 4 + col;
                }
            }
        }
    }
    GPIO_KEY = 0x0f;
    return 16;
}

// === 新增：定时器0初始化 ===
void Init_Timer0() {
    TMOD |= 0x01; // 设置定时器0为模式1 (16位定时器)
    // 设置定时时间为 20ms (假设晶振12MHz)
    // 20ms = 20000us. 65536 - 20000 = 45536 = 0xB1E0
    TH0 = 0xB1; 
    TL0 = 0xE0;
    ET0 = 1; // 开启定时器0中断
    EA = 1;  // 开启总中断
    TR0 = 1; // 启动定时器
}

// === 新增：定时器0中断服务程序 ===
// 每 20ms 自动执行一次，代替了主循环中的扫描
void Timer0_ISR() interrupt 1 {
    u8 key = 0;
    u8 tens, units;
    u8 tens_rev, units_rev;

    // 重装初值，保证下一次也是 20ms 后触发
    TH0 = 0xB1;
    TL0 = 0xE0;

    // 执行键盘扫描
    key = KeyScan();

    // 如果扫描到了按键 (不等于16)，则更新显示
    if (key != 16) {
        // 1. 拆分数字
        tens = key / 10;
        units = key % 10;            
        // 2. 软件修复接线反转 (查表法)
        tens_rev = rev_map[tens];
        units_rev = rev_map[units];            
        // 3. 组合输出
        GPIO_DIG = (units_rev << 4) | tens_rev;
    }
}

void main() {
    GPIO_DIG = 0x00; // 初始化显示
    
    Init_Timer0();   // 初始化定时器中断

    // 主循环现在是空的！
    // CPU 可以在这里处理其他任务，或者休眠
    // 只有当定时器时间到了，才会跳去处理键盘，符合“提高效率”的要求
    while (1) {
        
    }
}