// Watchdog : hardware IWDG. Call pet() every loop; if the loop hangs and stops, the chip reboots itself.
#ifndef WATCHDOG_H
#define WATCHDOG_H
#include "stm32f4xx_hal.h"

class Watchdog
{
  public:
    void init()
    {
        hiwdg.Instance       = IWDG;
        hiwdg.Init.Prescaler = IWDG_PRESCALER_32;   // ~32 kHz LSI / 32 -> ~1 ms per count, so Reload ~= ms
        hiwdg.Init.Reload    = TIMEOUT_MS;
        HAL_IWDG_Init(&hiwdg);
    }

    void pet() { HAL_IWDG_Refresh(&hiwdg); }

  private:
    static const uint32_t TIMEOUT_MS = 2000;   // ~2 s (12-bit counter, max ~4095)
    IWDG_HandleTypeDef hiwdg = {};
};
#endif
