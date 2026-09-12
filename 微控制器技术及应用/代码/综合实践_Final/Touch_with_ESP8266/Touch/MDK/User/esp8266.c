#include "esp8266.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ===================== UART3 Ring Buffer ===================== */
#define ESP8266_RX_BUF_SIZE 1024

static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static uint8_t s_rx_buf[ESP8266_RX_BUF_SIZE];

static uint8_t s_work_state = 0; /* 0=待机，1=工作 */

static char s_ip_str[20] = "0.0.0.0";
/* Shared scratch buffers to keep stack usage low (single-threaded). */
static char s_at_buf[512];
static char s_http_header[256];
static char s_http_body[1024];

/* ===================== UART Low-level ===================== */
static void UART3_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /* PB10=TX, PB11=RX */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);
}

static void UART3_Init(uint32_t baud)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    UART3_GPIO_Init();

    USART_InitStructure.USART_BaudRate            = baud;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);
}

void USART3_IRQHandler(void)
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        uint8_t ch = (uint8_t)USART_ReceiveData(USART3);
        uint16_t next = (uint16_t)(s_rx_head + 1);
        if(next >= ESP8266_RX_BUF_SIZE) next = 0;

        if(next != s_rx_tail) /* drop if full */
        {
            s_rx_buf[s_rx_head] = ch;
            s_rx_head = next;
        }
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

static int RB_Available(void)
{
    if(s_rx_head >= s_rx_tail) return (int)(s_rx_head - s_rx_tail);
    return (int)(ESP8266_RX_BUF_SIZE - s_rx_tail + s_rx_head);
}

static int RB_ReadByte(uint8_t *out)
{
    if(s_rx_head == s_rx_tail) return 0;
    *out = s_rx_buf[s_rx_tail];
    s_rx_tail++;
    if(s_rx_tail >= ESP8266_RX_BUF_SIZE) s_rx_tail = 0;
    return 1;
}

static void UART_SendBytes(const uint8_t *buf, uint16_t len)
{
    for(uint16_t i=0;i<len;i++)
    {
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {}
        USART_SendData(USART3, buf[i]);
    }
}

static void UART_SendString(const char *s)
{
    UART_SendBytes((const uint8_t*)s, (uint16_t)strlen(s));
}

/* ===================== AT Command Helpers ===================== */
static uint32_t s_ms_tick = 0;

static void ms_tick_delay(uint32_t ms)
{
    /* 项目中已有delay_ms，但这里为了更可控，仍然用它即可 */
    delay_ms((u16)ms);
}

static void RB_DrainTo(char *dst, uint16_t dst_cap, uint16_t *io_len)
{
    uint16_t len = *io_len;
    uint8_t b;
    while(RB_ReadByte(&b))
    {
        if(len < (uint16_t)(dst_cap - 1))
        {
            dst[len++] = (char)b;
        }
        else
        {
            /* overflow: shift left half */
            memmove(dst, dst + dst_cap/2, dst_cap/2);
            len = dst_cap/2;
            dst[len++] = (char)b;
        }
    }
    dst[len] = '\0';
    *io_len = len;
}

static int Wait_For(const char *expect, uint32_t timeout_ms)
{
    char *buf = s_at_buf;
    uint16_t len = 0;
    buf[0] = '\0';

    uint32_t waited = 0;
    while(waited < timeout_ms)
    {
        RB_DrainTo(buf, sizeof(s_at_buf), &len);
        if(strstr(buf, expect) != NULL) return 1;
        ms_tick_delay(10);
        waited += 10;
    }
    return 0;
}

static int Send_AT(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    /* 清空接收缓冲 */
    while(RB_Available() > 0)
    {
        uint8_t dump;
        RB_ReadByte(&dump);
    }

    UART_SendString(cmd);
    UART_SendString("\r\n");

    if(expect == NULL) return 1;
    return Wait_For(expect, timeout_ms);
}

static void ESP8266_HW_Reset(void)
{
#if ESP8266_USE_HW_RESET
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = ESP8266_RST_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(ESP8266_RST_GPIO, &GPIO_InitStructure);

    GPIO_ResetBits(ESP8266_RST_GPIO, ESP8266_RST_PIN);
    delay_ms(50);
    GPIO_SetBits(ESP8266_RST_GPIO, ESP8266_RST_PIN);
    delay_ms(500);
#endif
}

static void Parse_IP_From_CIFSR(const char *resp)
{
    /* 常见返回： +CIFSR:STAIP,"192.168.1.23" */
    const char *p = strstr(resp, "STAIP");
    if(!p) p = strstr(resp, "APIP");
    if(!p) return;

    p = strchr(p, '"');
    if(!p) return;
    p++;
    const char *q = strchr(p, '"');
    if(!q) return;

    size_t n = (size_t)(q - p);
    if(n >= sizeof(s_ip_str)) n = sizeof(s_ip_str) - 1;
    memcpy(s_ip_str, p, n);
    s_ip_str[n] = '\0';
}

static void Get_IP(void)
{
    char *buf = s_at_buf;
    uint16_t len = 0;
    buf[0] = '\0';

    /* 清空接收缓冲 */
    while(RB_Available() > 0)
    {
        uint8_t dump;
        RB_ReadByte(&dump);
    }

    UART_SendString("AT+CIFSR\r\n");

    uint32_t waited = 0;
    while(waited < 1500)
    {
        RB_DrainTo(buf, sizeof(s_at_buf), &len);
        if(strstr(buf, "OK") != NULL)
        {
            Parse_IP_From_CIFSR(buf);
            return;
        }
        ms_tick_delay(10);
        waited += 10;
    }
}

/* ===================== HTTP Server ===================== */
static void ESP8266_SendHttp(uint8_t id, const char *content_type, const char *body, uint8_t need_refresh)
{
    int body_len = (int)strlen(body);
    int header_len = 0;

    if(need_refresh)
    {
        header_len = snprintf(s_http_header, sizeof(s_http_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n"
                 "Cache-Control: no-cache\r\n"
                 "Pragma: no-cache\r\n"
                 "Refresh: %d\r\n"
                 "\r\n",
                 content_type, body_len, (int)(ESP8266_HTTP_REFRESH_MS/1000));
    }
    else
    {
        header_len = snprintf(s_http_header, sizeof(s_http_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n"
                 "Cache-Control: no-cache\r\n"
                 "Pragma: no-cache\r\n"
                 "\r\n",
                 content_type, body_len);
    }

    if(header_len < 0 || header_len >= (int)sizeof(s_http_header)) return;

    int send_len = header_len + body_len;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", id, send_len);
    if(!Send_AT(cmd, ">", 1500)) return;

    UART_SendBytes((const uint8_t*)s_http_header, (uint16_t)header_len);
    UART_SendBytes((const uint8_t*)body, (uint16_t)body_len);
    Wait_For("SEND OK", 2000);

    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d", id);
    Send_AT(cmd, "OK", 1000);
}




static void ESP8266_HandleHttp(uint8_t id, const char *req, uint16_t req_len)
{
    (void)req_len;

    if(strstr(req, "GET /state") != NULL)
    {
        snprintf(s_http_body, sizeof(s_http_body), "%d\r\n", s_work_state ? 1 : 0);
        ESP8266_SendHttp(id, "text/plain; charset=gb2312", s_http_body, 0);
    }
    else
    {
        /* 默认主页：JS 每5s轮询 /state */
        snprintf(s_http_body, sizeof(s_http_body),
                 "<!doctype html><html><head><meta charset='gb2312'>"
                 "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<title>STM32 状态</title></head>"
                 "<body style='font-family:Arial;padding:16px;'>"
                 "<h2>STM32 工作状态</h2>"
                 "<div>当前：<span id='s'>-</span></div>"
                 "<div style='margin-top:8px;color:#666;font-size:14px;'>"
                 "说明：工作模式=1，待机模式=0（每5秒自动更新）</div>"
                 "<script>"
                 "async function tick(){"
                 "try{let r=await fetch('/state',{cache:'no-store'});"
                 "let t=await r.text();"
                 "document.getElementById('s').innerText=t.trim();"
                 "}catch(e){document.getElementById('s').innerText='ERR';}"
                 "}"
                 "tick(); setInterval(tick,%d);"
                 "</script>"
                 "</body></html>",
                 (int)ESP8266_HTTP_REFRESH_MS);
        ESP8266_SendHttp(id, "text/html; charset=gb2312", s_http_body, 0);
    }
}

void ESP8266_SetWorkState(uint8_t is_work)
{
    s_work_state = (is_work ? 1 : 0);
}

const char* ESP8266_GetIP(void)
{
    return s_ip_str;
}

void ESP8266_Init(void)
{
    UART3_Init(115200);
    delay_ms(200);

    /* 建议把ESP8266的EN/CH_PD、GPIO0、GPIO2上拉，GPIO15下拉，保证正常启动 */
    ESP8266_HW_Reset();

    /* 复位并关闭回显 */
    Send_AT("AT", "OK", 800);
    Send_AT("ATE0", "OK", 800);
    Send_AT("AT+RST", "ready", 3000);

    /* WiFi STA 模式 */
    Send_AT("AT+CWMODE=1", "OK", 1000);

    /* 连接路由器 */
    {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ESP8266_WIFI_SSID, ESP8266_WIFI_PASSWORD);
        /* 连接可能需要几秒 */
        Send_AT(cmd, "WIFI GOT IP", 10000);
        Wait_For("OK", 2000);
    }

    /* 获取IP */
    Get_IP();

    /* 开启多连接 + TCP Server 80 */
    Send_AT("AT+CIPMUX=1", "OK", 1000);
    Send_AT("AT+CIPSERVER=1,80", "OK", 1000);

    /* 关闭透传（确保可解析 +IPD） */
    Send_AT("AT+CIPMODE=0", "OK", 1000);
}

void ESP8266_Task(void)
{
    static char accum[1536];
    static uint16_t accum_len = 0;

    /* 取出串口数据到accum */
    RB_DrainTo(accum, sizeof(accum), &accum_len);

    /* 解析 +IPD */
    char *p = strstr(accum, "+IPD,");
    if(!p) return;

    /* 解析格式：+IPD,<id>,<len>:<data> */
    int id = -1;
    int len = -1;

    char *comma1 = strchr(p, ',');
    if(!comma1) return;
    char *comma2 = strchr(comma1+1, ',');
    if(!comma2) return;
    char *colon  = strchr(comma2+1, ':');
    if(!colon) return;

    id  = atoi(comma1+1);
    len = atoi(comma2+1);
    if(id < 0 || id > 4 || len <= 0) goto consume_some;

    /* 判断是否已接收完整data */
    uint16_t prefix_len = (uint16_t)(colon + 1 - accum);
    if(accum_len < (uint16_t)(prefix_len + len)) return; /* not enough yet */

    const char *data = colon + 1;

    /* 复制请求到临时缓冲并处理 */
    static char req[768];
    uint16_t copy_len = (len < (int)(sizeof(req)-1)) ? (uint16_t)len : (uint16_t)(sizeof(req)-1);
    memcpy(req, data, copy_len);
    req[copy_len] = '\0';

    ESP8266_HandleHttp((uint8_t)id, req, copy_len);

consume_some:
    /* 消费掉已处理/部分数据，避免accum无限增长 */
    {
        /* 把p之前的内容和本次IPD段一起丢弃到colon+1+len */
        char *end = colon;
        if(len > 0) end = (char*)(colon + 1 + len);
        if(end > accum + accum_len) end = accum + accum_len;

        uint16_t remain = (uint16_t)(accum + accum_len - end);
        memmove(accum, end, remain);
        accum_len = remain;
        accum[accum_len] = '\0';
    }
}
