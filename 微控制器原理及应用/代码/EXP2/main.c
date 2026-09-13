#include <REGX51.H>
#include <intrins.h>
void delay(unsigned int t)
{
    unsigned int i, j;
    for (i = 0; i < t; i++)
    {
        for (j = 0; j < 120; j++);
    }
}

void System_Init()
{
    P2 = (P2 & 0x1f) | 0xa0;
    P0 = 0x00;
    P2 &= 0x1f;
    P2 = (P2 & 0x1f) | 0x80;
    P0 = 0xff;
    P2 &= 0x1f;
}

void main()
{
    // 预定义的模式数组
    // 0x81 -> 1000 0001 (两端)
    // 0x42 -> 0100 0010 (向内)
    // 0x24 -> 0010 0100 (再向内)
    // 0x18 -> 0001 1000 (中间相遇)
    unsigned char patterns[] = {0x81, 0x42, 0x24, 0x18, 0x24, 0x42};
    unsigned char i;
    unsigned char pattern_count = sizeof(patterns) / sizeof(patterns[0]);

    System_Init();

    while (1)
    {
        for (i = 0; i < pattern_count; i++)
        {
            P0 = ~patterns[i]; // 从数组中取值
            P2 = (P2 & 0x1f) | 0x80;
            P2 &= 0x1f;
            delay(250); // 调整每个状态的停留时间
        }
    }
}