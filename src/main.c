/*
 * STM32-EMG-Biosignal : Stage D+, EMG envelope with adaptive bias + mains rejection.
 *
 * Pipeline, all on-device, runs in a 1 kHz timer interrupt:
 *   1. timer-triggered ADC sample at a known fixed rate (no more sloppy 20 Hz polling)
 *   2. ADAPTIVE bias removal : the DC center is tracked live, NOT hardcoded, so it
 *      self-centers in any outlet / building / supply. (Can's catch: 1850 was fragile.)
 *   3. 50 Hz NOTCH on the linear AC signal, BEFORE rectifying. This is the real mains
 *      removal. Doing it after rectification (an earlier mistake) cannot work: rectifying
 *      folds the 50 Hz into a DC envelope level that no averaging can take back out.
 *   4. rectify : abs(notched AC)
 *   5. moving average : smooth the rectified signal into the envelope
 *
 * Why this is robust to conditions: the bias is measured, not assumed; and the contraction
 * is an AC swing AROUND the center (Can's data: rest mean ~1847, contracted mean ~1852),
 * so tracking the center never erases a contraction.
 *
 * The ONE location-specific constant is the mains frequency. Turkey = 50 Hz everywhere.
 * Moving to Canada (60 Hz): set SAMPLE_HZ 1200 and MAINS_HZ 60 (keeps WIN = 20). See below.
 */
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ---- sampling / mains constants (the only location-dependent knobs) ---------------- */
#define SAMPLE_HZ   1000               /* ADC sample rate. Canada 60 Hz: use 1200.        */
#define MAINS_HZ    50                 /* local mains frequency. Canada: 60.              */
#define WIN         (SAMPLE_HZ/MAINS_HZ)   /* envelope smoother length, samples (20)      */
#define BIAS_ALPHA  0.001f             /* bias tracker speed: ~1 s settle at 1 kHz.       */
                                       /* slow enough to ignore EMG, fast enough for drift */

/* 50 Hz notch biquad (RBJ), designed for f0/fs = 50/1000 = 0.05, Q = 2.5.
 * Because SAMPLE_HZ/MAINS_HZ is kept at 20, f0/fs stays 0.05 in Canada too (60/1200),
 * so these same coefficients retarget to 60 Hz automatically. Q is low-ish on purpose:
 * a wide notch still lands on 50 Hz despite the HSI clock being ~1% off. */
#define NOTCH_B0   0.9417923f
#define NOTCH_B1  -1.7913934f
#define NOTCH_B2   0.9417923f
#define NOTCH_A1  -1.7913934f
#define NOTCH_A2   0.8835919f

static ADC_HandleTypeDef  hadc1;
static UART_HandleTypeDef huart2;
static TIM_HandleTypeDef  htim2;

/* shared with the main loop : written in ISR, read for printing */
static volatile float g_bias = 2048.0f;   /* live DC center, seeds mid-scale, converges fast */
static volatile float g_env  = 0.0f;       /* latest envelope value                            */

/* moving-average ring buffer for the envelope */
static float    ring[WIN];
static int      ridx = 0;
static float    rsum = 0.0f;

/* diagnostic : raw signal range + clip count per print window (set at 1 kHz in the ISR) */
static volatile uint16_t win_min  = 4095;
static volatile uint16_t win_max  = 0;
static volatile uint16_t clip_cnt = 0;   /* samples pinned near a rail (0 or 4095)        */
static volatile uint16_t samp_cnt = 0;

/* 50 Hz notch, direct-form-I biquad. State persists between samples. */
static float nx1 = 0, nx2 = 0, ny1 = 0, ny2 = 0;
static float notch(float x)
{
    float y = NOTCH_B0 * x + NOTCH_B1 * nx1 + NOTCH_B2 * nx2
                           - NOTCH_A1 * ny1 - NOTCH_A2 * ny2;
    nx2 = nx1; nx1 = x;
    ny2 = ny1; ny1 = y;
    return y;
}

static uint32_t emg_read_raw(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 2);   /* conversion is ~24 us, well inside 1 ms */
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;
}

/* fires at SAMPLE_HZ (1 kHz) : the whole DSP pipeline lives here */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

    uint32_t rawi = emg_read_raw();

    /* diagnostic : capture the true swing + clipping at full sample rate */
    if (rawi < win_min) win_min = (uint16_t)rawi;
    if (rawi > win_max) win_max = (uint16_t)rawi;
    if (rawi <= 1 || rawi >= 4094) clip_cnt++;
    samp_cnt++;

    float raw = (float)rawi;

    /* (2) adaptive DC center : tracks ~1850 wherever it is */
    g_bias += BIAS_ALPHA * (raw - g_bias);
    float ac = raw - g_bias;            /* DC removed : AC = EMG + 50 Hz mains */

    /* (3) 50 Hz notch on the LINEAR signal, before rectifying : the real mains removal */
    float acf = notch(ac);

    /* (4) rectify the notched signal */
    float rect = fabsf(acf);

    /* (5) moving average : smooth into the envelope */
    rsum      -= ring[ridx];
    ring[ridx] = rect;
    rsum      += rect;
    ridx       = (ridx + 1) % WIN;
    g_env      = rsum / (float)WIN;
}

static void adc_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0;            /* PA0 = A0 */
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_0;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);
}

static void uart_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/* TIM2 at SAMPLE_HZ. Timer clock is 16 MHz (default HSI, APB1 prescaler 1). */
static void timer_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 16 - 1;                 /* 16 MHz / 16 = 1 MHz tick   */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = (1000000 / SAMPLE_HZ) - 1;  /* 1 MHz / 1000 = 1 kHz  */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_TIM_Base_Start_IT(&htim2);
}

void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }

int main(void)
{
    HAL_Init();
    for (int i = 0; i < WIN; i++) ring[i] = 0.0f;

    adc_init();
    uart_init();
    timer_init();

    char line[96];   /* roomy : the diagnostic line is ~50 chars, was overflowing at 48 */
    const char *hello = "STM32-EMG : adaptive bias + mains notch. env rises on contraction.\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)hello, strlen(hello), 100);

    while (1)
    {
        /* snapshot + reset the diagnostic window (tiny race with ISR, fine for a probe) */
        uint16_t mn = win_min, mx = win_max, cl = clip_cnt, sc = samp_cnt;
        win_min = 4095; win_max = 0; clip_cnt = 0; samp_cnt = 0;

        uint32_t bias = (uint32_t)g_bias;
        uint32_t env  = (uint32_t)g_env;
        int n = snprintf(line, sizeof line,
                         "raw=%4u..%4u clip=%3u/%4u bias=%4lu env=%4lu\r\n",
                         mn, mx, cl, sc, (unsigned long)bias, (unsigned long)env);
        if (n < 0) n = 0;
        if (n > (int)sizeof line) n = (int)sizeof line;   /* never transmit past the buffer */
        HAL_UART_Transmit(&huart2, (uint8_t *)line, n, 100);
        HAL_Delay(50);   /* print at 20 Hz; the DSP itself runs at 1 kHz in the ISR */
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
