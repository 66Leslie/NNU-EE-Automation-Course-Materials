#include <REGX51.H>

unsigned char Seg_Buf[8] = {2, 1, 2, 3, 0, 9, 3, 3};
unsigned char code seg_dula[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0xff};

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = ms; i > 0; i--)
        for (j = 800; j > 0; j--); // 大致延时
}

void Seg_Disp(unsigned char wela, unsigned char dula) {
    // 位选
    P0 = 0x01 << wela;
    P2 = (P2 & 0x1f) | 0xc0; // Y6C
    P2 &= 0x1f;

    // 段选
    P0 = seg_dula[dula];
    P2 = (P2 & 0x1f) | 0xe0; // Y7C
    P2 &= 0x1f;

    // 短暂延时以保证亮度
    delay_ms(1);

    // 消隐，防止重影
    P0 = 0xff;
    P2 = (P2 & 0x1f) | 0xe0; // Y7C
    P2 &= 0x1f;
}

void main() {
    unsigned char i;

    // 关闭蜂鸣器和继电器
    P0 = 0x00;
    P2 = (P2 & 0x1f) | 0xa0; // Y5C
    P2 &= 0x1f;
    
    while (1) {
        for (i = 0; i < 8; i++) {
            Seg_Disp(i, Seg_Buf[i]);
        }
    }
}