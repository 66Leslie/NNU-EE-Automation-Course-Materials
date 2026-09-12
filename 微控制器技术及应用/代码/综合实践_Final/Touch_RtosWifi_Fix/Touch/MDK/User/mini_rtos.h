#ifndef __MINI_RTOS_H
#define __MINI_RTOS_H

#include "stm32f4xx.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 配置 ===================== */
#define RTOS_MAX_TASKS          4   /* 含Idle在内，实际可用任务数=RTOS_MAX_TASKS-1 */
#define RTOS_TICK_HZ            1000u

typedef void (*rtos_task_func_t)(void *arg);

/* 任务创建：stack_words 为 uint32_t 数量（不是字节数） */
int  rtos_create_task(rtos_task_func_t entry, void *arg, uint32_t *stack, uint32_t stack_words, const char *name);
void rtos_start(void);

/* 任务延时/让出（单位：ms / tick） */
void rtos_delay_ms(uint32_t ms);
void rtos_yield(void);

/* 运行状态 */
uint32_t rtos_get_tick(void);
uint8_t  rtos_is_running(void);

/* 供中断/端口层调用 */
void RTOS_Tick_Handler(void);
void *RTOS_SelectNextTask(void); /* 返回下一任务的TCB指针 */

/* 端口层（汇编）提供 */
void RTOS_SVC_Handler(void);
void RTOS_PendSV_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MINI_RTOS_H */
