#include "delay.h"
#include "stm32f4xx.h"

/*
 * Stable delay implementation
 * - SysTick interrupt provides a 1ms time base: g_systick_ms
 * - DWT CYCCNT provides microsecond delay
 *
 * IMPORTANT: SysTick must NOT be reprogrammed inside delay_ms().
 */

volatile uint32_t g_systick_ms = 0;
static uint32_t s_cycles_per_us = 0;

static void DWT_DelayInit(void)
{
    /* Enable DWT CYCCNT */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_init(void)
{
    SystemCoreClockUpdate();
    s_cycles_per_us = SystemCoreClock / 1000000U;

    DWT_DelayInit();

    /* Configure SysTick for 1ms interrupts */
    SysTick_Config(SystemCoreClock / 1000U);
}

uint32_t delay_get_ms(void)
{
    return g_systick_ms;
}

void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * s_cycles_per_us;

    while ((uint32_t)(DWT->CYCCNT - start) < ticks)
    {
        __NOP();
    }
}

void delay_ms(uint16_t ms)
{
    uint32_t start = delay_get_ms();
    while ((uint32_t)(delay_get_ms() - start) < (uint32_t)ms)
    {
        __NOP();
    }
}
