#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ===================== 用户需要修改的WiFi参数 ===================== */
// #define ESP8266_WIFI_SSID        "vivo S16"
// #define ESP8266_WIFI_PASSWORD    "18962991856"
#define ESP8266_WIFI_SSID        "for_test"
#define ESP8266_WIFI_PASSWORD    "12345678"

/* ===================== 串口选择：USART3 (PB10/PB11) ===================== 
   - PB10 -> USART3_TX -> 连接到 ESP8266_RXD
   - PB11 -> USART3_RX -> 连接到 ESP8266_TXD
   注意：ESP8266为3.3V IO，严禁直接接5V。 */

/* 可选：硬件复位脚（若你把ESP8266的RST接到STM32） */
#define ESP8266_USE_HW_RESET     1
#if ESP8266_USE_HW_RESET
#define ESP8266_RST_GPIO         GPIOB
#define ESP8266_RST_PIN          GPIO_Pin_1   /* PB1 -> ESP8266_RST（低有效） */
#endif

/* 轮询刷新周期（浏览器自动刷新/JS轮询周期），单位ms */
#define ESP8266_HTTP_REFRESH_MS  1000

void ESP8266_Init(void);
void ESP8266_Task(void);
void ESP8266_SetWorkState(uint8_t is_work);
void ESP8266_SetTelemetry(int16_t temp_x10, uint16_t hum_x10, uint8_t speed);
void ESP8266_SetTime(uint8_t hour, uint8_t min, uint8_t sec);
void ESP8266_TimeTick1ms(void);
uint8_t ESP8266_TimeIsValid(void);
void ESP8266_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec);
const char* ESP8266_GetIP(void);

#endif
