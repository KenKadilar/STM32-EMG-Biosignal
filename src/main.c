/*
 * STM32-EMG-Biosignal : scaffold main.
 *
 * Placeholder firmware for the SCAFFOLD stage (board not in hand yet). It builds
 * against framework = stm32cube so the toolchain is proven end to end, locally and
 * in CI, before the Nucleo-F446RE arrives. Real Phase 0 work (clock config, blink
 * LD2 on PA5, then FreeRTOS) replaces this.
 *
 * Project scope : ../ChatAssistants/HealthAssistant/CareerAssistant/STM32_Project_Scope.md
 */
#include "stm32f4xx_hal.h"

int main(void)
{
    HAL_Init();

    /* TODO Phase 0 : SystemClock_Config(), then blink LD2 (PA5) via HAL_GPIO. */
    /* TODO Phase 1 : hand off to FreeRTOS (sampler / processor / comms tasks). */

    while (1)
    {
        /* nothing yet : scaffold only */
    }
}

/* HAL time base. Without this the startup weak handler is a dead loop and any
 * HAL_Delay would hang. Cheap to keep correct from day one. */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
