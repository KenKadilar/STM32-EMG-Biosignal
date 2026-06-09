#ifndef TIMER_H
#define TIMER_H
#include "stm32f4xx_hal.h"

// Timer : loop metronome (waitForNextTick, drift-free) + a plain blocking pause(). See FIRMWARE_MAP.md.
class Timer
{
  public:
    void initialLoopTickStarter() { nextDeadline = HAL_GetTick(); }   // call once, after HAL_Init

    void waitForNextTick(uint32_t periodMillis)
    {
        while ((int32_t)(nextDeadline - HAL_GetTick()) > 0) { }   // spin until the deadline
        nextDeadline += periodMillis;                             // advance to the next one
    }

    // Block for waitTimeMillis, then return. (HAL_Delay() is the built-in equivalent.)
    void pause(uint32_t waitTimeMillis)
    {
        uint32_t endTime = HAL_GetTick() + waitTimeMillis;
        while ((int32_t)(endTime - HAL_GetTick()) > 0) { }   // spin until we reach endTime
    }

  private:
    uint32_t nextDeadline = 0;   // set for real by initialLoopTickStarter()
};
#endif
