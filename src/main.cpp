// main.cpp : the standalone myoelectric gripper under FreeRTOS. servoTask (~200 Hz) + commsTask (~50 Hz
// serial telemetry) + canTask (~5 Hz CAN status heartbeat) + watchdogTask. The 1 kHz brain runs in the
// DMA callback and hands each flex to servoTask through the mailBox queue. File map: FIRMWARE_MAP.md
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Emg.h"
#include "Servo.h"
#include "Comms.h"
#include "MuscleTrigger.h"
#include "Watchdog.h"
#include "Mcp2515CanBus.h"

extern "C" void xPortSysTickHandler(void);   // the kernel tick, defined in the ARM_CM4F port

// the parts (built cheaply here; real setup happens in init())
static Emg           emg;
static Servo         servo;
static Comms         comms;
static MuscleTrigger trigger;
static Watchdog      watchdog;
static Mcp2515CanBus canBus;

// The brain (DMA ISR) drops one token in here per valid flex; servoTask waits on it. This replaces the
// old volatile flag: a queue is the safe ISR->task hand-off, it does the locking, so no manual volatile.
static QueueHandle_t mailBox;

// servoTask : highest priority. Waits on the flex queue, toggles on a flex, eases every cycle.
static void servoTask(void *params)
{
    (void)params;
    uint8_t token;
    while (1)
    {
        // block up to 5 ms for a flex; the timeout doubles as the ~200 Hz pacing (replaces vTaskDelay)
        if (xQueueReceive(mailBox, &token, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            servo.toggle();   // a flex arrived
        }
        servo.ease();         // glide toward the target every cycle
    }
}

// commsTask : telemetry to the laptop viewer. ~50 Hz is plenty (the viewer refreshes ~30/sec).
static void commsTask(void *params)
{
    (void)params;
    while (1)
    {
        uint16_t raw      = emg.read();
        int      centered = trigger.centered();
        comms.sendStatus(raw, centered, trigger.isElectrodeAttached());
        vTaskDelay(pdMS_TO_TICKS(20));   // ~50 Hz
    }
}

// canTask : broadcast a CAN status heartbeat (~5 Hz) so other nodes know the gripper's state. Two bytes,
// [gripper closed? 0/1][signal valid? 0/1], under status ID 0x101. SPI2 to the MCP2515 is canTask's alone,
// so no locking is needed.
static void canTask(void *params)
{
    (void)params;
    while (1)
    {
        uint8_t status[2] = { (uint8_t)(servo.isClosed()  ? 1 : 0),
                              (uint8_t)(trigger.isElectrodeAttached() ? 1 : 0) };
        canBus.sendFrame(0x101, status, 2);
        vTaskDelay(pdMS_TO_TICKS(200));   // ~5 Hz heartbeat
    }
}

// watchdogTask : lowest priority. Pets the IWDG every 500 ms (well inside the ~2 s timeout). Being lowest
// IS the safety: if a higher task hangs and never yields, this never runs -> never pets -> the chip reboots.
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
    watchdog.init();     // arm the IWDG; watchdogTask feeds it

    // CAN: bring up the MCP2515 over SPI2 and go live on the bus (Normal mode). canTask broadcasts status.
    canBus.init();
    canBus.reset();
    canBus.setBitTiming8MHz500k();   // 8 MHz crystal @ 500 kbps; must match the other node(s)
    canBus.enterNormalMode();

    mailBox = xQueueCreate(4, sizeof(uint8_t));   // up to 4 pending flexes, 1 byte each
    configASSERT(mailBox != NULL);                // out of heap -> stop here so it's obvious

    // priorities (higher = more urgent; idle is 0): servo preempts comms/can preempts watchdog.
    xTaskCreate(servoTask,    "servo",    256, nullptr, 3, nullptr);   // highest: actuation must not be delayed
    xTaskCreate(commsTask,    "comms",    512, nullptr, 2, nullptr);   // serial telemetry
    xTaskCreate(canTask,      "can",      256, nullptr, 2, nullptr);   // CAN status heartbeat
    xTaskCreate(watchdogTask, "watchdog", 128, nullptr, 1, nullptr);   // lowest: starved if anything above hangs
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

// The 1 kHz brain. On a valid flex, hand a token to servoTask via the queue, then yield so the higher-
// priority servoTask runs the instant this ISR exits. The DMA interrupt is priority 5 (the syscall
// ceiling), so calling xQueueSendFromISR from here is allowed.
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *h)
{
    if (mailBox != NULL && trigger.update(emg.read()))   // mailBox is NULL until the scheduler is set up
    {
        uint8_t token = 1;
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(mailBox, &token, &higherPriorityTaskWoken);
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}
extern "C" void DMA2_Stream0_IRQHandler(void) { emg.handleDmaIrq(); }
