#include "mini_rtos.h"

/* TCB类型在 mini_rtos.c 内部定义，这里只声明当前TCB指针 */
extern void *g_current_tcb;

/* 端口层：PendSV 完成上下文切换 */
__asm void RTOS_PendSV_Handler(void)
{
    PRESERVE8
    IMPORT g_current_tcb
    IMPORT RTOS_SelectNextTask

    CPSID   I

    MRS     R0, PSP            ; R0 = PSP
    CBZ     R0, pend_exit      ; PSP=0 说明还未启动（保险）

    STMDB   R0!, {R4-R11}      ; 保存 R4-R11 到当前任务栈

    LDR     R1, =g_current_tcb
    LDR     R2, [R1]           ; R2 = current_tcb
    STR     R0, [R2]           ; current_tcb->sp = R0

    PUSH    {LR}
    BL      RTOS_SelectNextTask
    POP     {LR}

    LDR     R1, =g_current_tcb
    STR     R0, [R1]           ; g_current_tcb = next_tcb

    LDR     R0, [R0]           ; R0 = next_tcb->sp
    LDMIA   R0!, {R4-R11}      ; 恢复 R4-R11
    MSR     PSP, R0            ; PSP = new sp

pend_exit
    CPSIE   I
    BX      LR
}

/* 端口层：SVC 启动第一任务 */
__asm void RTOS_SVC_Handler(void)
{
    PRESERVE8
    IMPORT g_current_tcb

    LDR     R0, =g_current_tcb
    LDR     R0, [R0]           ; R0 = current_tcb
    LDR     R0, [R0]           ; R0 = current_tcb->sp

    LDMIA   R0!, {R4-R11}      ; 恢复 R4-R11
    MSR     PSP, R0            ; PSP 指向硬件栈帧

    MOVS    R0, #2
    MSR     CONTROL, R0        ; 使用 PSP，特权线程
    ISB

    LDR     LR, =0xFFFFFFFD    ; 异常返回到线程模式，使用PSP
    BX      LR
}
