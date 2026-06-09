// main.cpp : wires the parts together and runs the ~200 Hz loop. File layout: FIRMWARE_MAP.md
#include "stm32f4xx_hal.h"
#include "Emg.h"
#include "Servo.h"
#include "Comms.h"
#include "Timer.h"

// the parts, as global objects (built cheaply here; real setup happens in init()/the starter)
static Emg   emg;
static Servo servo;
static Comms comms;
static Timer timer;

int main(void)
{
    HAL_Init();          // wake up the HAL (the 1 ms tick, flash settings)
    emg.init();
    servo.init();
    comms.init();
    timer.initialLoopTickStarter();   // start the loop timer now that HAL's clock is running

    while (1)
    {
        uint16_t raw = emg.read();                    // one muscle sample (0..4095)
        comms.sendDataCOM(raw);                       // ship it to the laptop
        servo.rotateTowards(comms.receivedDataForServo());   // ease toward the latest command

        timer.waitForNextTick(5);                     // do the work, then wait out the rest of the 5 ms
    }
}

/* ---- hardware interrupt handlers --------------------------------------------------------
 * The chip jumps to these by their EXACT names when an event happens. extern "C" stops C++
 * from renaming them, so the chip's jump-table still finds them. Each just pokes an object.
 */
extern "C" void SysTick_Handler(void)   { HAL_IncTick(); }           // every 1 ms: keep HAL's clock ticking
extern "C" void USART2_IRQHandler(void) { comms.onByteReceived(); }  // a serial byte just arrived
