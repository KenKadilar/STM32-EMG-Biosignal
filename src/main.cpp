// main.cpp : B2 , the EMG work split into tasks: servoTask (~200 Hz actuation) + commsTask (~50 Hz telemetry) + watchdogTask. The 1 kHz brain stays in the DMA callback. Priorities get tuned in Step C. File map: FIRMWARE_MAP.md
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Emg.h"
#include "Servo.h"
#include "Comms.h"
#include "MuscleTrigger.h"
#include "Watchdog.h"

extern "C" void xPortSysTickHandler(void);   // the kernel tick, defined in the ARM_CM4F port

// the parts (built cheaply here; real setup happens in init())
static Emg           emg;
static Servo         servo;
static Comms         comms;
static MuscleTrigger trigger;
static Watchdog      watchdog;

static volatile bool toggleRequested = false;   // the 1 kHz brain sets this; servoTask acts on it

// servoTask : the time-critical actuation. ~200 Hz so the gripper eases smoothly.
static void servoTask(void *params)
{
    (void)params;
    while (1)
    {
        if (toggleRequested) { servo.toggle(); toggleRequested = false; }   // the brain flagged a flex
        servo.ease();                                                       // glide toward the target
        vTaskDelay(pdMS_TO_TICKS(5));                                       // ~200 Hz
    }
}

// commsTask : telemetry to the laptop viewer. ~50 Hz is plenty (the viewer refreshes ~30/sec).
static void commsTask(void *params)
{
    (void)params;
    while (1)
    {
        uint16_t raw      = emg.read();                          // freshest sample
        int      centered = trigger.centered();                  // latest centered value from the brain
        comms.sendStatus(raw, centered, trigger.isValid());      // stream raw,centered,valid
        vTaskDelay(pdMS_TO_TICKS(20));                           // ~50 Hz
    }
}

// watchdogTask : keep the IWDG fed. Petting every 500 ms is well inside the ~2 s timeout.
// Step C drops this to the LOWEST priority, so a hung higher task starves it -> the chip reboots.
static void watchdogTask(void *params)
{
    (void)params;
    while (1)
    {
        watchdog.pet();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    HAL_Init();          // HAL + its 1 ms SysTick
    emg.init();
    servo.init();
    comms.init();
    watchdog.init();     // arm the IWDG; watchdogTask now feeds it

    // all priority 1 for now; Step C sets the real priorities (servo high, comms mid, watchdog lowest)
    xTaskCreate(servoTask,    "servo",    256, nullptr, 1, nullptr);
    xTaskCreate(commsTask,    "comms",    512, nullptr, 1, nullptr);   // 512 words: sendStatus formats a string
    xTaskCreate(watchdogTask, "watchdog", 128, nullptr, 1, nullptr);
    vTaskStartScheduler();                                             // hand the CPU to the kernel; does NOT return

    while (1) { }        // only reached if the scheduler could not start
}

// Interrupt handlers. SysTick feeds BOTH HAL and the kernel. The 1 kHz brain runs in the ADC/DMA callback.
extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) { xPortSysTickHandler(); }
}
extern "C" void USART2_IRQHandler(void)  { comms.onByteReceived(); }
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *h) { if (trigger.update(emg.read())) toggleRequested = true; }
extern "C" void DMA2_Stream0_IRQHandler(void)                  { emg.handleDmaIrq(); }
