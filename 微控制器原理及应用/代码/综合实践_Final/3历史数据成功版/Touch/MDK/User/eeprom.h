#ifndef __EEPROM_H
#define __EEPROM_H

#include "stm32f4xx.h"

/*
 * 24Cxx EEPROM 软件I2C驱动（标准库）
 *
 * 你当前硬件：24C02（内部地址 8bit）
 *  - EEPROM_ADDR_BYTES = 1
 *  - EEPROM_PAGE_SIZE  = 8
 *
 * 接线（两线I2C）：
 *   SCL(你说的SCK) -> PB8
 *   SDA            -> PB9
 *   还必须接 VCC/GND；SCL/SDA 必须有上拉（模块无上拉需外接约4.7k到VCC）
 */

/* ===================== 用户可配置区 ===================== */

/* 24C02：1字节地址 */
#ifndef EEPROM_ADDR_BYTES
#define EEPROM_ADDR_BYTES   1
#endif

/* 7位设备地址：A2/A1/A0=0 时通常为 0x50 */
#ifndef EEPROM_I2C_ADDR7
#define EEPROM_I2C_ADDR7    0x50
#endif

/* 软件I2C引脚（默认 PB8/PB9） */
#ifndef EEPROM_SCL_GPIO
#define EEPROM_SCL_GPIO     GPIOB
#endif
#ifndef EEPROM_SCL_PIN
#define EEPROM_SCL_PIN      GPIO_Pin_8
#endif

#ifndef EEPROM_SDA_GPIO
#define EEPROM_SDA_GPIO     GPIOB
#endif
#ifndef EEPROM_SDA_PIN
#define EEPROM_SDA_PIN      GPIO_Pin_9
#endif

/* 24C02 页写大小：8字节 */
#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE    8
#endif

/* 软件I2C延时：越大越慢越稳 */
#ifndef EEPROM_I2C_DELAY
#define EEPROM_I2C_DELAY    80
#endif

/* ===================== 通用接口 ===================== */

void EEPROM_Init(void);

/* 返回0表示成功；非0表示失败（无ACK/通信错误等） */
u8 EEPROM_ReadBytes(u16 mem_addr, u8 *buf, u16 len);
u8 EEPROM_WriteBytes(u16 mem_addr, const u8 *buf, u16 len);

/* 简单自检：写入/读回固定模式，返回0通过，非0失败 */
u8 EEPROM_SelfTest(void);

/* ===================== 兼容旧工程接口 ===================== */
/* 你 main.c 里用的是 EEPROM24C64_xxx（历史遗留命名），这里做兼容映射
 * 注意：即便命名叫24C64，这里实际也支持 24C02
 */
void EEPROM24C64_Init(void);
u8   EEPROM24C64_Read(u16 mem_addr, u8 *buf, u16 len);
u8   EEPROM24C64_Write(u16 mem_addr, const u8 *buf, u16 len);

#endif
