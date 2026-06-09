#ifndef EMG_H
#define EMG_H
#include "stm32f4xx_hal.h"

// Emg : the muscle sensor (ADC on PA0). read() is a homemade analogRead.
class Emg
{
  public:
    void init()
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();      // turn on power to port A (PA0 lives here)
        __HAL_RCC_ADC1_CLK_ENABLE();       // turn on power to the ADC

        // set PA0 to "analog" so the ADC can read the raw voltage on it
        GPIO_InitTypeDef g = {0};
        g.Pin = GPIO_PIN_0; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &g);

        // configure the ADC: 12-bit, one single reading at a time, started by software
        hadc.Instance                   = ADC1;
        hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
        hadc.Init.Resolution            = ADC_RESOLUTION_12B;     // result is 0..4095
        hadc.Init.ScanConvMode          = DISABLE;
        hadc.Init.ContinuousConvMode    = DISABLE;
        hadc.Init.DiscontinuousConvMode = DISABLE;
        hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
        hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
        hadc.Init.NbrOfConversion       = 1;
        hadc.Init.DMAContinuousRequests = DISABLE;
        hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
        HAL_ADC_Init(&hadc);

        // pick the input (channel 0 = PA0) and how long to sample it
        ADC_ChannelConfTypeDef ch = {0};
        ch.Channel = ADC_CHANNEL_0; ch.Rank = 1; ch.SamplingTime = ADC_SAMPLETIME_84CYCLES;
        HAL_ADC_ConfigChannel(&hadc, &ch);
    }

    // One reading: start the conversion -> wait for it -> grab the number -> stop.
    uint16_t read()
    {
        HAL_ADC_Start(&hadc);
        HAL_ADC_PollForConversion(&hadc, 2);          // wait up to 2 ms
        uint16_t value = (uint16_t)HAL_ADC_GetValue(&hadc);
        HAL_ADC_Stop(&hadc);
        return value;
    }

  private:
    ADC_HandleTypeDef hadc = {};   // this sensor's ADC config + live state
};
#endif
