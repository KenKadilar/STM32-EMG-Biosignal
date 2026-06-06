/*
 * STM32-EMG-Biosignal : Stage 5 bring-up, SG90 servo sweep.
 *
 * Drives a 50 Hz PWM on PB6 (TIM4_CH1 = Nucleo header D10) and sweeps the pulse width
 * between 1.0 ms and 2.0 ms, so an SG90 servo swings back and forth. Confirms the servo
 * works, the timer PWM is right, and your separate-supply + shared-ground rig is correct.
 *
 * No EMG here : this is an isolated actuator test. Later we replace the sweep with the
 * gesture output (open / close) from the EMG envelope.
 *
 * PWM math: TIM4 on APB1 = 16 MHz (default HSI). Prescaler /16 -> 1 MHz tick (1 us).
 * Period 20000 ticks -> 20 ms -> 50 Hz. Compare value = pulse width in microseconds.
 *   ~1000 us = one end, ~1500 us = middle, ~2000 us = other end.
 *
 * WIRING (see docs/wiring.md):
 *   Servo signal (orange) -> PB6 / D10
 *   Servo V+    (red)      -> SEPARATE 4xAAA (6 V) pack, NOT the Nucleo 5V pin
 *   Servo GND   (brown)    -> battery minus AND Nucleo GND (shared ground is mandatory)
 * The STM32's 3.3 V PWM drives the servo fine regardless of the 6 V motor supply.
 */
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define SERVO_MIN_US  1000     /* clones vary; widen toward 600..2400 later if needed */
#define SERVO_MAX_US  2000

static UART_HandleTypeDef huart2;
static TIM_HandleTypeDef  htim4;

static void uart_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3; gpio.Mode = GPIO_MODE_AF_PP; gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH; gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200; huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1; huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX; huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/* TIM4_CH1 on PB6, 50 Hz PWM, compare value = microseconds */
static void servo_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_6;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOB, &gpio);

    htim4.Instance           = TIM4;
    htim4.Init.Prescaler     = 16 - 1;       /* 16 MHz / 16 = 1 MHz (1 us per tick) */
    htim4.Init.CounterMode   = TIM_COUNTERMODE_UP;
    htim4.Init.Period        = 20000 - 1;    /* 20000 us = 20 ms = 50 Hz            */
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim4);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 1500;                     /* start centered */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
}

int main(void)
{
    HAL_Init();
    uart_init();
    servo_init();

    char line[40];
    const char *hello = "STM32-EMG : SG90 sweep on PB6/D10 (1.0..2.0 ms)\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)hello, strlen(hello), 100);

    int us = SERVO_MIN_US, dir = 10;
    while (1)
    {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, us);

        int n = snprintf(line, sizeof line, "servo=%d us\r\n", us);
        HAL_UART_Transmit(&huart2, (uint8_t *)line, n, 50);

        us += dir;
        if (us >= SERVO_MAX_US) { us = SERVO_MAX_US; dir = -10; }
        if (us <= SERVO_MIN_US) { us = SERVO_MIN_US; dir =  10; }

        HAL_Delay(20);   /* 100 steps end-to-end -> ~2 s per sweep */
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
