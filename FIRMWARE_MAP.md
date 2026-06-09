# Firmware map

Navigation for `src/`. This is the single place that says what each source file does, so the
files themselves stay clean (no big header banners). One file = one part of the device.

Board: NUCLEO-F446RE. Chip: STM32F446RE. Language: C++ (header-only classes, one per part).
Build: PlatformIO (`framework = stm32cube`). Flash: `pio run -t upload`.

## Files (in `src/`)

| File | What it is |
|---|---|
| `main.cpp` | Creates the parts, runs the ~200 Hz loop, and holds the two hardware interrupt handlers (SysTick, USART2). |
| `Emg.h` | `Emg` : the muscle sensor. ADC on PA0. `read()` returns one 0..4095 sample. |
| `Servo.h` | `Servo` : the gripper. PWM on PB6 (TIM4 channel 1). `rotateTowards(us)` eases toward a target (clamped, never slams); `setRotationSpeed(1..20)` sets the step size. |
| `Comms.h` | `Comms` : USB serial to the laptop (USART2). `sendDataCOM(v)` streams a sample; the interrupt `onByteReceived()` collects `S<us>` commands; `receivedDataForServo()` returns the latest. (`lastReceived()` / `lastText()` are debug peeks.) |
| `Timer.h` | `Timer` : `waitForNextTick(ms)` paces the loop at a steady rate with no drift; `pause(ms)` is a plain blocking wait. Call `initialLoopTickStarter()` once after `HAL_Init`. |

## The loop (`main.cpp`, ~200 times a second)

1. `emg.read()` : take one muscle sample.
2. `comms.sendDataCOM(raw)` : send it to the laptop.
3. `servo.rotateTowards(comms.receivedDataForServo())` : ease the gripper toward the latest command.
4. `timer.waitForNextTick(5)` : wait out the rest of the 5 ms (200 Hz).

The two interrupt handlers live at the bottom of `main.cpp`: `SysTick_Handler` keeps the HAL
millisecond clock; `USART2_IRQHandler` feeds each arriving byte to `comms.onByteReceived()`.

## Where the "thinking" is (today vs the goal)

Right now the open/close DECISION runs on the laptop (`tools/emg_studio/operate.py`): it reads
the stream, detects a muscle "dip", and sends back `S<us>` commands. The board just samples,
streams, and obeys. Moving that decision ONTO the chip (so it runs standalone, with the laptop
as just a viewer) is the next milestone.

## Pins

| Part | Pin | Notes |
|---|---|---|
| EMG sensor | PA0 (A0) | ADC1 analog input |
| Servo | PB6 (D10) | TIM4 channel 1, PWM at 50 Hz |
| Serial TX / RX | PA2 / PA3 | USART2, bridged to the one USB cable by the onboard ST-LINK (virtual COM port) |
| Onboard LED | PA5 (LD2) | available, not currently used |
