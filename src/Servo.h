#ifndef SERVO_H
#define SERVO_H
#include "stm32f4xx_hal.h"

// Servo : the gripper (SG90, PWM on PB6 / D10). Eases toward a clamped target; never slams.
class Servo
{
  public:
    void init()
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();      // turn on power to port B (PB6 lives here)
        __HAL_RCC_TIM4_CLK_ENABLE();       // turn on power to timer 4 (it makes the PWM)

        // hand PB6 over to timer 4 ("alternate function 2") so the timer can drive the pin
        GPIO_InitTypeDef g = {0};
        g.Pin = GPIO_PIN_6; g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL;
        g.Speed = GPIO_SPEED_FREQ_LOW; g.Alternate = GPIO_AF2_TIM4;
        HAL_GPIO_Init(GPIOB, &g);

        // tick once per microsecond (16 MHz / 16), roll over every 20 ms = 50 Hz servo frame
        htim.Instance           = TIM4;
        htim.Init.Prescaler     = 16 - 1;
        htim.Init.CounterMode   = TIM_COUNTERMODE_UP;
        htim.Init.Period        = 20000 - 1;
        htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        HAL_TIM_PWM_Init(&htim);

        // the PWM "compare" value IS the pulse width in microseconds; start at OPEN
        TIM_OC_InitTypeDef oc = {0};
        oc.OCMode = TIM_OCMODE_PWM1; oc.Pulse = OPEN; oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        HAL_TIM_PWM_ConfigChannel(&htim, &oc, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1);

        targetPosition = currentPosition = OPEN;
    }

    // The one call main uses each loop: aim at a position AND ease one step toward it.
    void rotateTowards(int positionMicros)
    {
        setTarget(positionMicros);   // move the goalpost (clamped to the safe range)
        easeOneStep();               // take one small step toward it
    }

    // Adjust how fast the gripper moves: microseconds of travel per step (bigger = faster).
    void setRotationSpeed(int speed)
    {
        slew = clampToRange(speed, 1, 20);
    }

  private:
    // Measured for OUR gripper (2026-06-08): 1300 us = open, 1600 us = almost closed.
    // NOTE: these are PWM pulse widths in MICROSECONDS, not degrees - the servo turns them into an angle.
    static const int MIN = 1270, MAX = 1620, OPEN = 1300;

    static int clampToRange(int value, int low, int high)
    {
        if (value < low)  return low;
        if (value > high) return high;
        return value;
    }

    void setTarget(int positionMicros)
    {
        targetPosition = clampToRange(positionMicros, MIN, MAX);
    }

    // Move at most 'slew' microseconds toward the target, then write it to the PWM.
    void easeOneStep()
    {
        int step = targetPosition - currentPosition;   // how far we still have to go
        if (step >  slew) step =  slew;                // ...but never more than 'slew' per call,
        if (step < -slew) step = -slew;                //    in either direction
        currentPosition += step;                       // (step is 0 when we're already there)
        __HAL_TIM_SET_COMPARE(&htim, TIM_CHANNEL_1, currentPosition);
    }

    TIM_HandleTypeDef htim = {};
    int slew = 4;                 // microseconds moved per step = rotation speed (see setRotationSpeed)
    int targetPosition  = OPEN;   // where we want to be (pulse width in us)
    int currentPosition = OPEN;   // where we are right now (eased toward the target)
};
#endif
