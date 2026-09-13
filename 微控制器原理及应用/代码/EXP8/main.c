#include <reg52.h>
#define uchar unsigned char
#define uint  unsigned int

// --- 1. 引脚定义 ---
// ADC 控制线
sbit ADC_Control = P3^4; // 请将 ADC的 START(Pin6) 和 ALE(Pin22) 都接到 P3.4
sbit OE          = P3^7; // 输出允许
// sbit EOC      = P3^2; // 转换结束 (本代码使用延时法，可不接)

// ADC 地址线 (根据你的图: C->P2.5, B->P2.6, A->P2.7)
sbit ADDC = P2^5;
sbit ADDB = P2^6;
sbit ADDA = P2^7;

// 数码管段码表 (共阳)
// 0-9 的段码
uchar code seg_code[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};
// 0.-9. 的段码 (带小数点)
uchar code seg_code_dot[] = {0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,0x00,0x10};

// --- 2. 全局变量 (用于定时器和主循环通信) ---
// 显示缓冲区：存放4个数码管当前应该显示的“段码”
uchar Display_Buffer[4] = {0xFF, 0xFF, 0xFF, 0xFF}; 
uchar scan_index = 0; // 当前扫描到第几位

// 延时函数
void delay_ms(uint ms)
{
    uint i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<110; j++);
}
// --- 3. 定时器0中断：专门负责数码管刷新 ---
void Timer0_ISR() interrupt 1
{
    // 重装初值 (12MHz晶振下，约2ms)
    TH0 = (65536 - 2000) / 256;
    TL0 = (65536 - 2000) % 256;

    // 1. 消影 (关闭位选)
    // 你的位选在 P2.0-P2.3，先只操作低4位
    P2 &= 0xF0; 

    // 2. 送段码 (从缓冲区取数据)
    P0 = Display_Buffer[scan_index];

    // 3. 送位选 (选中当前位)
    // 这里的逻辑是：保留P2高4位(ADC地址)，只改低4位
    // 假设是高电平选中 (根据之前的代码逻辑)
    P2 |= (0x01 << scan_index);

    // 4. 索引递增 (0->1->2->3->0)
    scan_index++;
    if(scan_index >= 4) scan_index = 0;
}
// --- 4. 更新显示缓冲区函数 ---
// 将计算好的 通道号 和 电压值 转换成段码填入数组
void Update_Display_Buffer(uchar ch, uint voltage)
{
    // 第1位：显示通道号 (0-7)
    Display_Buffer[0] = seg_code[ch]; 

    // 第2位：电压整数位 (带小数点)
    Display_Buffer[1] = seg_code_dot[voltage / 100];

    // 第3位：电压十分位
    Display_Buffer[2] = seg_code[voltage % 100 / 10];

    // 第4位：电压百分位
    Display_Buffer[3] = seg_code[voltage % 10];
}
// --- 5. ADC读取函数 ---
uchar Read_ADC(uchar ch)
{
    uchar val;

    // A. 设置地址 (P2.5 - P2.7)
    // 注意：因为定时器也在操作P2口(低4位)，这里只操作高3位是安全的
    // 但为了保险，可以临时关中断，不过位寻址通常没问题
    if(ch & 0x01) ADDA = 1; else ADDA = 0;
    if(ch & 0x02) ADDB = 1; else ADDB = 0;
    if(ch & 0x04) ADDC = 1; else ADDC = 0;

    // B. 启动转换时序 (START/ALE 脉冲)
    ADC_Control = 0;
    ADC_Control = 1; // 上升沿：锁存地址
    ADC_Control = 0; // 下降沿：开始转换

    // C. 等待转换 (这里可以用延时，因为显示由中断负责，延时不会导致闪烁！)
    delay_ms(1); // 等待1ms足够了

    // D. 读取数据
    P1 = 0xFF;   // 【关键】读之前先置1
    OE = 1;      // 打开输出
    val = P1;    // 读取
    OE = 0;      // 关闭
    return val;
}
void main()
{
    uchar current_channel = 0;
    uint adc_result;
    uint voltage_calc;
    uint loop_timer = 0;
    // --- 初始化定时器0 ---
    TMOD = 0x01; // 模式1：16位定时器
    TH0 = (65536 - 2000) / 256;
    TL0 = (65536 - 2000) % 256;
    EA = 1;  // 开总中断
    ET0 = 1; // 开定时器0中断
    TR0 = 1; // 启动定时器
    // 初始化控制脚
    ADC_Control = 0;
    OE = 0;
    while(1)
    {
        // 1. 读取 ADC
        adc_result = Read_ADC(current_channel);

        // 2. 计算电压 (放大100倍)
        // 5V / 255 ≈ 0.0196 -> *196/100
        voltage_calc = (uint)adc_result * 196 / 100;

        // 3. 更新显示内容 (写入全局缓冲区，定时器会自动搬到屏幕上)
        Update_Display_Buffer(current_channel, voltage_calc);

        // 4. 通道切换逻辑
        // 延时一段时间再切通道，方便观察
        delay_ms(500); // 延时0.5秒
        
        // 增加通道号
        current_channel++;
        if(current_channel > 7) current_channel = 0;
    }
}