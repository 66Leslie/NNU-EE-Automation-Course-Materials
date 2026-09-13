#include "esp8266.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * ESP8266 (AT firmware) driver for STM32F407
 *
 * Key changes vs. the previous version:
 * 1) HTTP response TX is now NON-BLOCKING (state machine).
 *    - The previous version used large stack buffers + blocking waits.
 *    - When a browser opened the board IP, it could trigger stack overflow
 *      (default stack was only 1KB) or long blocking waits that looked like
 *      a "deadlock".
 * 2) All large buffers are static (no large automatic arrays).
 * 3) SysTick-based timeouts (delay_get_ms()) are used.
 *
 * Result:
 * - Browser access will no longer hard-fault / freeze the whole board.
 * - /state returns "1" for work mode and "0" for standby.
 */

/* ===================== UART3 Ring Buffer ===================== */
#define ESP8266_RX_BUF_SIZE 1024

static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static uint8_t s_rx_buf[ESP8266_RX_BUF_SIZE];

static uint8_t s_work_state = 0; /* 0=standby, 1=work */
static int16_t s_temp_x10 = 0;
static uint16_t s_hum_x10 = 0;
static uint8_t s_speed = 0;
static char s_ip_str[20] = "0.0.0.0";
static volatile uint8_t s_time_valid = 0;
static volatile uint8_t s_time_h = 0;
static volatile uint8_t s_time_m = 0;
static volatile uint8_t s_time_s = 0;
static volatile uint16_t s_time_ms = 0;

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
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        uint8_t ch = (uint8_t)USART_ReceiveData(USART3);
        uint16_t next = (uint16_t)(s_rx_head + 1);
        if (next >= ESP8266_RX_BUF_SIZE) next = 0;

        if (next != s_rx_tail) /* drop if full */
        {
            s_rx_buf[s_rx_head] = ch;
            s_rx_head = next;
        }
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

static int RB_Available(void)
{
    if (s_rx_head >= s_rx_tail) return (int)(s_rx_head - s_rx_tail);
    return (int)(ESP8266_RX_BUF_SIZE - s_rx_tail + s_rx_head);
}

static int RB_ReadByte(uint8_t *out)
{
    if (s_rx_head == s_rx_tail) return 0;
    *out = s_rx_buf[s_rx_tail];
    s_rx_tail++;
    if (s_rx_tail >= ESP8266_RX_BUF_SIZE) s_rx_tail = 0;
    return 1;
}

static void UART_SendBytes(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {}
        USART_SendData(USART3, buf[i]);
    }
}

static void UART_SendString(const char *s)
{
    UART_SendBytes((const uint8_t*)s, (uint16_t)strlen(s));
}

/* ===================== AT Command Helpers (blocking, init-only) ===================== */
static void RB_Flush(void)
{
    while (RB_Available() > 0)
    {
        uint8_t dump;
        RB_ReadByte(&dump);
    }
}

static void RB_DrainTo(char *dst, uint16_t dst_cap, uint16_t *io_len)
{
    uint16_t len = *io_len;
    uint8_t b;

    while (RB_ReadByte(&b))
    {
        if (len < (uint16_t)(dst_cap - 1))
        {
            dst[len++] = (char)b;
        }
        else
        {
            /* overflow: keep the last half */
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
    static char buf[512];
    static uint16_t len = 0;

    uint32_t start = delay_get_ms();
    while ((uint32_t)(delay_get_ms() - start) < timeout_ms)
    {
        RB_DrainTo(buf, sizeof(buf), &len);
        if (strstr(buf, expect) != NULL) return 1;
    }
    return 0;
}

static int Send_AT(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    RB_Flush();
    UART_SendString(cmd);
    UART_SendString("\r\n");
    if (expect == NULL) return 1;
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
    delay_ms(600);
#endif
}

static void Parse_IP_From_CIFSR(const char *resp)
{
    const char *p = strstr(resp, "STAIP");
    if (!p) p = strstr(resp, "APIP");
    if (!p) return;

    p = strchr(p, '"');
    if (!p) return;
    p++;

    const char *q = strchr(p, '"');
    if (!q) return;

    size_t n = (size_t)(q - p);
    if (n >= sizeof(s_ip_str)) n = sizeof(s_ip_str) - 1;
    memcpy(s_ip_str, p, n);
    s_ip_str[n] = '\0';
}

static void Get_IP(void)
{
    static char buf[512];
    static uint16_t len = 0;

    RB_Flush();
    UART_SendString("AT+CIFSR\r\n");

    uint32_t start = delay_get_ms();
    while ((uint32_t)(delay_get_ms() - start) < 1500U)
    {
        RB_DrainTo(buf, sizeof(buf), &len);
        if (strstr(buf, "OK") != NULL)
        {
            Parse_IP_From_CIFSR(buf);
            return;
        }
    }
}

/* ===================== HTTP Server (non-blocking TX) ===================== */
#define HTTP_TX_BUF_SIZE   1400
#define HTTP_BODY_BUF_SIZE 1000

typedef enum
{
    HTTP_TX_IDLE = 0,
    HTTP_TX_WAIT_PROMPT,
    HTTP_TX_WAIT_SEND_OK,
    HTTP_TX_WAIT_CLOSE_OK
} http_tx_state_t;

static http_tx_state_t s_tx_state = HTTP_TX_IDLE;
static uint8_t  s_tx_id = 0;
static uint32_t s_tx_deadline_ms = 0;

static char     s_http_body[HTTP_BODY_BUF_SIZE];
static char     s_http_tx[HTTP_TX_BUF_SIZE];
static uint16_t s_http_tx_len = 0;

static void Accum_Consume(char *accum, uint16_t *accum_len, uint16_t n)
{
    if (n >= *accum_len)
    {
        *accum_len = 0;
        accum[0] = '\0';
        return;
    }
    memmove(accum, accum + n, (size_t)(*accum_len - n));
    *accum_len = (uint16_t)(*accum_len - n);
    accum[*accum_len] = '\0';
}

static void HttpTx_Reset(void)
{
    s_tx_state = HTTP_TX_IDLE;
    s_http_tx_len = 0;
}

static void HttpTx_Begin(uint8_t id, const char *content_type, const char *body)
{
    int body_len = (int)strlen(body);
    int header_len = snprintf(s_http_tx, sizeof(s_http_tx),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Content-Length: %d\r\n"
                              "Connection: close\r\n"
                              "Cache-Control: no-cache\r\n"
                              "Pragma: no-cache\r\n"
                              "\r\n",
                              content_type, body_len);
    if (header_len < 0) return;

    /* Append body */
    if ((size_t)header_len + (size_t)body_len >= sizeof(s_http_tx)) return;
    memcpy(s_http_tx + header_len, body, (size_t)body_len);
    s_http_tx_len = (uint16_t)(header_len + body_len);

    /* Send CIPSEND first; then wait for '>' prompt in background */
    char cmd[40];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", (int)id, (int)s_http_tx_len);
    UART_SendString(cmd);

    s_tx_id = id;
    s_tx_state = HTTP_TX_WAIT_PROMPT;
    s_tx_deadline_ms = delay_get_ms() + 1500U;
}

static void HttpTx_Process(char *accum, uint16_t *accum_len)
{
    if (s_tx_state == HTTP_TX_IDLE) return;

    uint32_t now = delay_get_ms();
    if ((int32_t)(now - s_tx_deadline_ms) > 0)
    {
        /* Timeout: try close to release the connection and reset state */
        char cmd[24];
        snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", (int)s_tx_id);
        UART_SendString(cmd);
        HttpTx_Reset();
        return;
    }

    if (s_tx_state == HTTP_TX_WAIT_PROMPT)
    {
        char *p = strchr(accum, '>');
        if (p != NULL)
        {
            /* Consume up to and including '>' */
            uint16_t consume = (uint16_t)((p - accum) + 1);
            Accum_Consume(accum, accum_len, consume);

            UART_SendBytes((const uint8_t*)s_http_tx, s_http_tx_len);
            s_tx_state = HTTP_TX_WAIT_SEND_OK;
            s_tx_deadline_ms = now + 2500U;
        }
    }
    else if (s_tx_state == HTTP_TX_WAIT_SEND_OK)
    {
        char *ok = strstr(accum, "SEND OK");
        char *fail = strstr(accum, "SEND FAIL");
        char *err = strstr(accum, "ERROR");

        if (ok != NULL)
        {
            /* Consume through "SEND OK" */
            uint16_t consume = (uint16_t)((ok - accum) + strlen("SEND OK"));
            Accum_Consume(accum, accum_len, consume);

            char cmd[24];
            snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", (int)s_tx_id);
            UART_SendString(cmd);

            s_tx_state = HTTP_TX_WAIT_CLOSE_OK;
            s_tx_deadline_ms = now + 1000U;
        }
        else if (fail != NULL || err != NULL)
        {
            char cmd[24];
            snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", (int)s_tx_id);
            UART_SendString(cmd);
            s_tx_state = HTTP_TX_WAIT_CLOSE_OK;
            s_tx_deadline_ms = now + 1000U;
        }
    }
    else /* HTTP_TX_WAIT_CLOSE_OK */
    {
        /* For CIPCLOSE, ESP8266 may return "OK" or "CLOSED" */
        if (strstr(accum, "CLOSED") != NULL || strstr(accum, "OK") != NULL)
        {
            HttpTx_Reset();
        }
    }
}

static void ESP8266_HandleHttp(uint8_t id, const char *req)
{
    if (strstr(req, "GET /time") != NULL)
    {
        int hh = -1, mm = -1, ss = -1;
        const char *q = strchr(req, '?');

        if (q != NULL)
        {
            const char *p = q + 1;
            while (*p && *p != ' ' && *p != '\r' && *p != '\n')
            {
                if (strncmp(p, "hh=", 3) == 0) hh = atoi(p + 3);
                else if (strncmp(p, "mm=", 3) == 0) mm = atoi(p + 3);
                else if (strncmp(p, "ss=", 3) == 0) ss = atoi(p + 3);

                p = strchr(p, '&');
                if (!p) break;
                p++;
            }
        }

        if (hh >= 0 && hh < 24 && mm >= 0 && mm < 60 && ss >= 0 && ss < 60)
        {
            ESP8266_SetTime((uint8_t)hh, (uint8_t)mm, (uint8_t)ss);
            snprintf(s_http_body, sizeof(s_http_body), "OK\r\n");
        }
        else
        {
            snprintf(s_http_body, sizeof(s_http_body), "ERR\r\n");
        }

        HttpTx_Begin(id, "text/plain; charset=utf-8", s_http_body);
    }
    else if (strstr(req, "GET /data") != NULL)
    {
        int16_t t = s_temp_x10;
        int t_abs = (t < 0) ? -t : t;
        int t_int = t_abs / 10;
        int t_dec = t_abs % 10;
        unsigned int h_int = (unsigned int)(s_hum_x10 / 10);
        unsigned int h_dec = (unsigned int)(s_hum_x10 % 10);

        snprintf(s_http_body, sizeof(s_http_body),
                 "{\"state\":%d,\"temp\":%s%d.%d,\"humidity\":%u.%u,\"speed\":%u}\r\n",
                 s_work_state ? 1 : 0,
                 (t < 0) ? "-" : "", t_int, t_dec,
                 h_int, h_dec,
                 s_speed);
        HttpTx_Begin(id, "application/json; charset=utf-8", s_http_body);
    }
    else if (strstr(req, "GET /state") != NULL)
    {
        snprintf(s_http_body, sizeof(s_http_body), "%d\r\n", s_work_state ? 1 : 0);
        HttpTx_Begin(id, "text/plain; charset=utf-8", s_http_body);
    }
    else
    {
        /* Default page: browser polls /data every ESP8266_HTTP_REFRESH_MS */
        snprintf(s_http_body, sizeof(s_http_body),
                 "<!doctype html><html><head><meta charset='utf-8'>"
                 "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<title>STM32 Telemetry</title></head>"
                 "<body style='font-family:Arial;padding:16px;'>"
                 "<h2>STM32 Telemetry</h2>"
                 "<div>State: <span id='s'>-</span></div>"
                 "<div>Temp: <span id='t'>-</span> C</div>"
                 "<div>Humidity: <span id='h'>-</span> %</div>"
                 "<div>Speed: <span id='sp'>-</span> %%</div>"
                 "<div style='margin-top:8px;color:#666;font-size:14px;'>"
                 "auto update every %d ms</div>"
                 "<script>"
                 "async function tick(){"
                 "try{let r=await fetch('/data',{cache:'no-store'});"
                 "let j=await r.json();"
                 "document.getElementById('s').innerText=j.state;"
                 "document.getElementById('t').innerText=j.temp;"
                 "document.getElementById('h').innerText=j.humidity;"
                 "document.getElementById('sp').innerText=j.speed;"
                 "}catch(e){document.getElementById('s').innerText='ERR';}}"
                 "tick();setInterval(tick,%d);"
                 "</script></body></html>",
                 (int)ESP8266_HTTP_REFRESH_MS,
                 (int)ESP8266_HTTP_REFRESH_MS);

        HttpTx_Begin(id, "text/html; charset=utf-8", s_http_body);
    }
}

void ESP8266_SetWorkState(uint8_t is_work)
{
    s_work_state = (is_work ? 1 : 0);
}

void ESP8266_SetTelemetry(int16_t temp_x10, uint16_t hum_x10, uint8_t speed)
{
    s_temp_x10 = temp_x10;
    s_hum_x10 = hum_x10;
    s_speed = speed;
}

void ESP8266_SetTime(uint8_t hour, uint8_t min, uint8_t sec)
{
    if (hour >= 24 || min >= 60 || sec >= 60) return;
    s_time_h = hour;
    s_time_m = min;
    s_time_s = sec;
    s_time_ms = 0;
    s_time_valid = 1;
}

void ESP8266_TimeTick1ms(void)
{
    if (!s_time_valid) return;

    s_time_ms++;
    if (s_time_ms >= 1000)
    {
        s_time_ms = 0;
        s_time_s++;
        if (s_time_s >= 60)
        {
            s_time_s = 0;
            s_time_m++;
            if (s_time_m >= 60)
            {
                s_time_m = 0;
                s_time_h++;
                if (s_time_h >= 24) s_time_h = 0;
            }
        }
    }
}

uint8_t ESP8266_TimeIsValid(void)
{
    return s_time_valid;
}

void ESP8266_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    if (hour) *hour = s_time_h;
    if (min) *min = s_time_m;
    if (sec) *sec = s_time_s;
}

const char* ESP8266_GetIP(void)
{
    return s_ip_str;
}

void ESP8266_Init(void)
{
    UART3_Init(115200);
    delay_ms(200);

    /* Recommended boot straps: EN/GPIO0/GPIO2 pull-up, GPIO15 pull-down */
    ESP8266_HW_Reset();

    /* Basic init */
    Send_AT("AT", "OK", 800);
    Send_AT("ATE0", "OK", 800);
    Send_AT("AT+RST", "ready", 4000);

    /* WiFi STA mode */
    Send_AT("AT+CWMODE=1", "OK", 1200);

    /* Join AP */
    {
        char cmd[160];
        snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ESP8266_WIFI_SSID, ESP8266_WIFI_PASSWORD);
        Send_AT(cmd, "WIFI GOT IP", 15000);
        Wait_For("OK", 3000);
    }

    /* IP */
    Get_IP();

    /* Enable multiple connections + TCP server on port 80 */
    Send_AT("AT+CIPMUX=1", "OK", 1200);
    Send_AT("AT+CIPSERVER=1,80", "OK", 1200);

    /* Ensure normal mode to get +IPD */
    Send_AT("AT+CIPMODE=0", "OK", 1200);

    HttpTx_Reset();
}

void ESP8266_Task(void)
{
    static char accum[2048];
    static uint16_t accum_len = 0;

    /* Drain UART RX to accum */
    RB_DrainTo(accum, sizeof(accum), &accum_len);

    /* Process non-blocking TX state machine first (prompt / SEND OK / close) */
    HttpTx_Process(accum, &accum_len);

    /* Parse +IPD (incoming HTTP request) */
    while (1)
    {
        char *p = strstr(accum, "+IPD,");
        if (!p) break;

        /* +IPD,<id>,<len>:<data> */
        char *comma1 = strchr(p, ',');
        if (!comma1) break;
        char *comma2 = strchr(comma1 + 1, ',');
        if (!comma2) break;
        char *colon  = strchr(comma2 + 1, ':');
        if (!colon) break;

        int id  = atoi(comma1 + 1);
        int len = atoi(comma2 + 1);
        if (id < 0 || id > 4 || len <= 0)
        {
            /* Consume a little to avoid being stuck */
            Accum_Consume(accum, &accum_len, (uint16_t)(comma1 - accum + 1));
            continue;
        }

        uint16_t prefix_len = (uint16_t)(colon + 1 - accum);
        if (accum_len < (uint16_t)(prefix_len + (uint16_t)len))
        {
            /* Not complete yet */
            break;
        }

        /* Copy request data (limit for parsing) */
        static char req[768];
        uint16_t copy_len = (len < (int)(sizeof(req) - 1)) ? (uint16_t)len : (uint16_t)(sizeof(req) - 1);
        memcpy(req, colon + 1, copy_len);
        req[copy_len] = '\0';

        /* Consume the whole IPD segment (prefix + data len) */
        Accum_Consume(accum, &accum_len, (uint16_t)(prefix_len + (uint16_t)len));

        /* Handle request only if TX is idle.
         * If a new request comes in while we are still sending a response,
         * we just ignore it (browser will retry on next poll). */
        if (s_tx_state == HTTP_TX_IDLE)
        {
            ESP8266_HandleHttp((uint8_t)id, req);
        }

        /* Continue loop to parse the next IPD (if any) */
    }
}
