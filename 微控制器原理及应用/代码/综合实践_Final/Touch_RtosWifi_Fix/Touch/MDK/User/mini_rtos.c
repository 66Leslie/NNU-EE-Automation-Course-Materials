#include "mini_rtos.h"
#include <string.h>

/* ===================== 任务控制块 ===================== */
typedef enum {
    TASK_UNUSED = 0,
    TASK_READY  = 1,
    TASK_BLOCKED= 2
} task_state_t;

typedef struct
{
    uint32_t    *sp;
    uint32_t    *stack_base;
    uint32_t     stack_words;
    volatile uint32_t delay;
    task_state_t state;
    const char  *name;
} tcb_t;

/* 当前任务指针：供汇编端口层访问 */
tcb_t *g_current_tcb = 0;

static tcb_t  s_tasks[RTOS_MAX_TASKS];
static uint32_t s_task_count = 0;
static volatile uint32_t s_tick = 0;
static volatile uint8_t  s_running = 0;

/* Idle 任务 */
static void idle_task(void *arg)
{
    (void)arg;
    while(1)
    {
        __WFI();
    }
}

static void task_exit(void)
{
    /* 不允许任务函数直接return */
    while(1)
    {
        rtos_delay_ms(1000);
    }
}

/* 初始化任务栈：构造“异常返回”所需的栈帧 */
static uint32_t* init_stack(uint32_t *stack, uint32_t stack_words, rtos_task_func_t entry, void *arg)
{
    uint32_t *sp = stack + stack_words;

    /* 8字对齐（ARM EABI 建议） */
    sp = (uint32_t*)((uint32_t)sp & (~0x7u));

    /* 自动入栈的硬件栈帧（R0-R3,R12,LR,PC,xPSR） */
    *(--sp) = 0x01000000u;           /* xPSR (Thumb) */
    *(--sp) = (uint32_t)entry;       /* PC */
    *(--sp) = (uint32_t)task_exit;   /* LR */
    *(--sp) = 0x12121212u;           /* R12 */
    *(--sp) = 0x03030303u;           /* R3 */
    *(--sp) = 0x02020202u;           /* R2 */
    *(--sp) = 0x01010101u;           /* R1 */
    *(--sp) = (uint32_t)arg;         /* R0 */

    /* PendSV 手动入栈的 R4-R11 */
    *(--sp) = 0x11111111u;           /* R11 */
    *(--sp) = 0x10101010u;           /* R10 */
    *(--sp) = 0x09090909u;           /* R9  */
    *(--sp) = 0x08080808u;           /* R8  */
    *(--sp) = 0x07070707u;           /* R7  */
    *(--sp) = 0x06060606u;           /* R6  */
    *(--sp) = 0x05050505u;           /* R5  */
    *(--sp) = 0x04040404u;           /* R4  */

    return sp; /* 指向 R4 */
}

static int alloc_tcb(void)
{
    for(int i=0;i<RTOS_MAX_TASKS;i++)
    {
        if(s_tasks[i].state == TASK_UNUSED) return i;
    }
    return -1;
}

int rtos_create_task(rtos_task_func_t entry, void *arg, uint32_t *stack, uint32_t stack_words, const char *name)
{
    if(entry == 0 || stack == 0 || stack_words < 128) return -1;

    int idx = alloc_tcb();
    if(idx < 0) return -1;

    memset(&s_tasks[idx], 0, sizeof(s_tasks[idx]));
    s_tasks[idx].stack_base  = stack;
    s_tasks[idx].stack_words = stack_words;
    s_tasks[idx].sp          = init_stack(stack, stack_words, entry, arg);
    s_tasks[idx].delay       = 0;
    s_tasks[idx].state       = TASK_READY;
    s_tasks[idx].name        = name;

    s_task_count++;
    return idx;
}

static void rtos_create_idle(void)
{
    static uint32_t idle_stack[256];
    /* Idle 放在最后一个空位 */
    rtos_create_task(idle_task, 0, idle_stack, sizeof(idle_stack)/sizeof(idle_stack[0]), "idle");
}

/* 选择下一个 READY 任务：Round-robin */
void *RTOS_SelectNextTask(void)
{
    if(g_current_tcb == 0)
    {
        /* 找到第一个READY任务 */
        for(int i=0;i<RTOS_MAX_TASKS;i++)
        {
            if(s_tasks[i].state == TASK_READY)
                return &s_tasks[i];
        }
        return 0;
    }

    int cur = (int)(g_current_tcb - &s_tasks[0]);
    for(int k=1;k<=RTOS_MAX_TASKS;k++)
    {
        int i = (cur + k) % RTOS_MAX_TASKS;
        if(s_tasks[i].state == TASK_READY)
            return &s_tasks[i];
    }

    /* 理论上不会到这里（至少有Idle） */
    return g_current_tcb;
}

void RTOS_Tick_Handler(void)
{
    s_tick++;

    /* 更新阻塞任务 */
    for(int i=0;i<RTOS_MAX_TASKS;i++)
    {
        if(s_tasks[i].state == TASK_BLOCKED && s_tasks[i].delay > 0)
        {
            s_tasks[i].delay--;
            if(s_tasks[i].delay == 0)
            {
                s_tasks[i].state = TASK_READY;
            }
        }
    }

    /* 简单时间片：每tick触发一次PendSV */
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

uint32_t rtos_get_tick(void)
{
    return s_tick;
}

uint8_t rtos_is_running(void)
{
    return s_running;
}

void rtos_delay_ms(uint32_t ms)
{
    if(!s_running)
    {
        /* 未启动调度：忙等 */
        for(uint32_t i=0;i<ms;i++)
        {
            for(volatile uint32_t j=0;j< (SystemCoreClock/8000u); j++) { __NOP(); }
        }
        return;
    }

    if(ms == 0)
    {
        rtos_yield();
        return;
    }

    __disable_irq();
    g_current_tcb->delay = ms;
    g_current_tcb->state = TASK_BLOCKED;
    __enable_irq();

    rtos_yield();
}

void rtos_yield(void)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* 启动调度器 */
void rtos_start(void)
{
    /* 若用户没有创建Idle，这里补上 */
    rtos_create_idle();

    /* 选择第一个任务 */
    g_current_tcb = (tcb_t*)RTOS_SelectNextTask();

    /* 设置 PendSV/SysTick 优先级为最低 */
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    NVIC_SetPriority(SysTick_IRQn, 0xFE);
    NVIC_SetPriority(SVCall_IRQn, 0xFD);

    /* 配置 SysTick: 1ms tick */
    SysTick_Config(SystemCoreClock / RTOS_TICK_HZ);

    s_running = 1;

    /* 通过 SVC 进入第一任务 */
    __asm("svc 0");
    while(1) {}
}
