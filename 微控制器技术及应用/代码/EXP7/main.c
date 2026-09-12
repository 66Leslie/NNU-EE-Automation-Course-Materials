#include <reg52.h>
#define GPIO_DIG P2
#define GPIO_KEY P1
typedef unsigned char u8;
typedef unsigned int u16;
// 查找表：用于修正硬件接线反转的问题 (0->0, 1->8, 2->4, 3->C ...)
// 这个表会自动把 P2.0 和 P2.3 的功能对调
u8 code rev_map[] = 
    {0x00,0x08,0x04,0x0C,0x02,0x0A,0x06,0x0E,0x01,0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F};
void delay(u16 i) {
    while (i--);
}
u8 KeyScan() {
    static const u8 row_drive[4] = {0xFE, 0xFD, 0xFB, 0xF7};
    static const u8 col_mask[4] = {0x80, 0x40, 0x20, 0x10};
    u8 row, col;
    GPIO_KEY = 0x0f;
    if ((GPIO_KEY & 0x0f) == 0x0f) {
        return 16; // no key pressed
    }
    delay(300); // debounce
    if ((GPIO_KEY & 0x0f) == 0x0f) {
        return 16;
    }
    for (row = 0; row < 4; row++) {
        GPIO_KEY = row_drive[row];
        if ((GPIO_KEY & 0xf0) != 0xf0) {
            for (col = 0; col < 4; col++) {
                if ((GPIO_KEY & col_mask[col]) == 0) {
                    GPIO_KEY = 0x0f; // release bus
                    return row * 4 + col;
                }
            }
        }
    }
    GPIO_KEY = 0x0f;
    return 16;
}
void main() {
    u8 key = 0;
    u8 tens, units;
    u8 tens_rev, units_rev;
    GPIO_DIG = 0x00; 
    while (1) {
        key = KeyScan();     
        if (key != 16) {
            // 1. 拆分数字
            tens = key / 10;
            units = key % 10;            
            // 2. 软件修复接线反转 (查表法)
            tens_rev = rev_map[tens];
            units_rev = rev_map[units];            
            // 3. 组合输出
            // units (个位) 放在高四位 -> 对应右边数码管
            // tens (十位) 放在低四位 -> 对应左边数码管
            GPIO_DIG = (units_rev << 4) | tens_rev;
        }
    }
}