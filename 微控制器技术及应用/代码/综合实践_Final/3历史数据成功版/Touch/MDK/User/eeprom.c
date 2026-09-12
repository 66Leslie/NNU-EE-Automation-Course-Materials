#include "eeprom.h"

/* ========= 软件I2C 基础层 ========= */

static void eep_delay(void)
{
    volatile int i;
    for(i=0;i<EEPROM_I2C_DELAY;i++) { __NOP(); }
}

static void sda_out_od(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = EEPROM_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;     /* 开漏输出，靠上拉拉高 */
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(EEPROM_SDA_GPIO, &GPIO_InitStructure);
}

static void sda_in_pu(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = EEPROM_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(EEPROM_SDA_GPIO, &GPIO_InitStructure);
}

static void scl_out_od(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = EEPROM_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(EEPROM_SCL_GPIO, &GPIO_InitStructure);
}

static void SDA_H(void){ GPIO_SetBits(EEPROM_SDA_GPIO, EEPROM_SDA_PIN); }
static void SDA_L(void){ GPIO_ResetBits(EEPROM_SDA_GPIO, EEPROM_SDA_PIN); }
static void SCL_H(void){ GPIO_SetBits(EEPROM_SCL_GPIO, EEPROM_SCL_PIN); }
static void SCL_L(void){ GPIO_ResetBits(EEPROM_SCL_GPIO, EEPROM_SCL_PIN); }
static u8   SDA_READ(void){ return GPIO_ReadInputDataBit(EEPROM_SDA_GPIO, EEPROM_SDA_PIN); }

static void i2c_start(void)
{
    sda_out_od();
    SDA_H(); SCL_H(); eep_delay();
    SDA_L(); eep_delay();
    SCL_L(); eep_delay();
}

static void i2c_stop(void)
{
    sda_out_od();
    SDA_L(); SCL_H(); eep_delay();
    SDA_H(); eep_delay();
}

static void i2c_send_bit(u8 b)
{
    sda_out_od();
    if(b) SDA_H(); else SDA_L();
    eep_delay();
    SCL_H(); eep_delay();
    SCL_L(); eep_delay();
}

/* 读1位：释放SDA让从机驱动 */
static u8 i2c_read_bit(void)
{
    u8 b;
    sda_in_pu();
    eep_delay();
    SCL_H(); eep_delay();
    b = SDA_READ();
    SCL_L(); eep_delay();
    return b;
}

static void i2c_send_byte(u8 dat)
{
    u8 i;
    for(i=0;i<8;i++)
    {
        i2c_send_bit((dat & 0x80) ? 1 : 0);
        dat <<= 1;
    }
}

/* ack: 0=ACK继续，1=NACK结束 */
static u8 i2c_read_byte(u8 ack)
{
    u8 i, dat=0;
    for(i=0;i<8;i++)
    {
        dat <<= 1;
        dat |= i2c_read_bit();
    }
    i2c_send_bit(ack ? 1 : 0);
    return dat;
}

/* ACK轮询：等待内部写周期结束 */
static u8 eep_wait_ready(void)
{
    u16 t = 0;
    while(t < 8000)
    {
        i2c_start();
        i2c_send_byte((EEPROM_I2C_ADDR7<<1) | 0); /* 写地址 */
        /* 读ACK位：ACK=0 */
        if(i2c_read_bit() == 0)
        {
            i2c_stop();
            return 0;
        }
        i2c_stop();
        t++;
    }
    return 1;
}

/* ========= 对外接口 ========= */

void EEPROM_Init(void)
{
    /* 默认 GPIOB，可按你的工程修改 RCC */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    scl_out_od();
    sda_out_od();
    SDA_H();
    SCL_H();
}

/* 写一段（自动按页拆分），返回0成功 */
u8 EEPROM_WriteBytes(u16 mem_addr, const u8 *buf, u16 len)
{
    u16 written = 0;
    u16 addr = mem_addr;

    /* 24C02地址范围：0x00~0xFF，越界直接报错 */
    if((addr + len) > 0x0100) return 10;

    if(eep_wait_ready()) return 1;

    while(written < len)
    {
        u16 page_rem = EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE);
        u16 chunk = (len - written);
        if(chunk > page_rem) chunk = page_rem;

        i2c_start();
        i2c_send_byte((EEPROM_I2C_ADDR7<<1) | 0); /* 写 */
        if(i2c_read_bit()) { i2c_stop(); return 2; }

#if (EEPROM_ADDR_BYTES == 2)
        i2c_send_byte((u8)(addr >> 8));
        if(i2c_read_bit()) { i2c_stop(); return 3; }
#endif
        i2c_send_byte((u8)(addr & 0xFF));
        if(i2c_read_bit()) { i2c_stop(); return 4; }

        for(u16 i=0;i<chunk;i++)
        {
            i2c_send_byte(buf[written+i]);
            if(i2c_read_bit()) { i2c_stop(); return 5; }
        }
        i2c_stop();

        if(eep_wait_ready()) return 6;

        addr += chunk;
        written += chunk;
    }
    return 0;
}

/* 读一段，返回0成功 */
u8 EEPROM_ReadBytes(u16 mem_addr, u8 *buf, u16 len)
{
    if(len == 0) return 0;
    if((mem_addr + len) > 0x0100) return 10;

    i2c_start();
    i2c_send_byte((EEPROM_I2C_ADDR7<<1) | 0); /* 写：先发内部地址 */
    if(i2c_read_bit()) { i2c_stop(); return 1; }

#if (EEPROM_ADDR_BYTES == 2)
    i2c_send_byte((u8)(mem_addr >> 8));
    if(i2c_read_bit()) { i2c_stop(); return 2; }
#endif
    i2c_send_byte((u8)(mem_addr & 0xFF));
    if(i2c_read_bit()) { i2c_stop(); return 3; }

    i2c_start();
    i2c_send_byte((EEPROM_I2C_ADDR7<<1) | 1);
    if(i2c_read_bit()) { i2c_stop(); return 4; }

    for(u16 i=0;i<len;i++)
    {
        buf[i] = i2c_read_byte((i == (len-1)) ? 1 : 0);
    }
    i2c_stop();
    return 0;
}

u8 EEPROM_SelfTest(void)
{
    u8 w[8] = {0x55,0xAA,0x12,0x34,0xA5,0x5A,0x00,0xFF};
    u8 r[8] = {0};

    /* 24C02总共256B，选一个不覆盖你头部数据的地址（0x80起） */
    if(EEPROM_WriteBytes(0x0080, w, sizeof(w))) return 1;
    if(EEPROM_ReadBytes (0x0080, r, sizeof(r))) return 2;

    for(u8 i=0;i<8;i++)
    {
        if(r[i] != w[i]) return 3;
    }
    return 0;
}

/* ========= 兼容旧接口：让 main.c 不用改函数名 ========= */

void EEPROM24C64_Init(void)
{
    EEPROM_Init();
}

u8 EEPROM24C64_Read(u16 mem_addr, u8 *buf, u16 len)
{
    return EEPROM_ReadBytes(mem_addr, buf, len);
}

u8 EEPROM24C64_Write(u16 mem_addr, const u8 *buf, u16 len)
{
    return EEPROM_WriteBytes(mem_addr, buf, len);
}
