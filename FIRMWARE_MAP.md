# Firmware map

Navigation for `src/` + the FreeRTOS layout. The single place that says what each piece does, so the
files stay clean (no big header banners). One file = one part of the device.

Board: NUCLEO-F446RE. Chip: STM32F446RE. Language: C++ (header-only classes) + the FreeRTOS kernel in
`lib/`. Build: PlatformIO (`framework = stm32cube`), **hard-float** (see platformio.ini + fpu_link.py).
Flash: `pio run -t upload`.

## Files (in `src/`)

| File | What it is |
|---|---|
| `main.cpp` | Creates the parts + the `mailBox` queue, creates the 3 FreeRTOS tasks, calls `vTaskStartScheduler()`, and holds the interrupt handlers: SysTick (chained to HAL + kernel), USART2, and the 1 kHz ADC/DMA brain callback. |
| `Emg.h` | `Emg` : the muscle sensor. TIM2 triggers the ADC on PA0 at 1 kHz and DMA parks each result in RAM; `read()` returns the freshest sample (no CPU per reading). |
| `MuscleTrigger.h` | `MuscleTrigger` : the on-chip brain. Tracks the resting baseline, centers the signal, and returns `true` once per valid flex (a "dip"). Includes the signal-loss failsafe. |
| `Servo.h` | `Servo` : the gripper. PWM on PB6 (TIM4 ch 1). `open()`/`close()`/`toggle()` choose a position; `ease()` glides toward it (slew-limited). |
| `Comms.h` | `Comms` : USB serial (USART2). `sendStatus(raw, centered, valid)` streams telemetry. (Old `S<us>` receive path present but unused.) |
| `Watchdog.h` | `Watchdog` : hardware IWDG (~2 s). `pet()` resets it; if nothing pets it, the chip reboots. |
| `Notch.h` | `Notch` : a 50 Hz biquad notch; `filter()` kills mains hum (used by MuscleTrigger). |
| `Timer.h` | **No longer used** under FreeRTOS, `vTaskDelay` and the queue's receive-timeout replaced `waitForNextTick`. Vestigial; safe to delete in a cleanup pass. |

## FreeRTOS layout

- `lib/FreeRTOS/` , the kernel (`tasks.c`/`list.c`/`queue.c`), the `ARM_CM4F` port (`port.c` +
  `portmacro.h`), `heap_4.c`, and the headers. Vendor code, copied from the stm32cube package, not edited.
- `include/FreeRTOSConfig.h` , OUR config (16 MHz HSI clock, 1 kHz tick, 10 KB heap, PendSV/SVC mapped
  to the port). In `include/` so both our code and the kernel see it; the vendor folder stays pure.
- Hard-float is mandatory for the CM4F port. The wiring is `platformio.ini` `build_flags` +
  `fpu_link.py` (the link half). Do NOT remove either, the build breaks. See HANDOFF.md.

## The 1 kHz brain + the three tasks

The brain runs in an **interrupt**, not a task. `HAL_ADC_ConvCpltCallback` fires at 1 kHz (every DMA
sample) and runs `trigger.update(emg.read())`: baseline + centering + notch + dip detection. On a valid
flex it `xQueueSendFromISR`s a token into `mailBox` and `portYIELD_FROM_ISR`s (so the higher-priority
servoTask runs the instant the ISR exits).

Three tasks (priorities: higher number = more urgent; idle is 0):
1. `servoTask` (**pri 3**, highest): `xQueueReceive(mailBox, ..., 5 ms)`. On a token -> `servo.toggle()`;
   `servo.ease()` every cycle. The 5 ms receive-timeout doubles as the ~200 Hz pacing.
2. `commsTask` (**pri 2**): every 20 ms (~50 Hz) -> `comms.sendStatus(raw, centered, valid)`.
3. `watchdogTask` (**pri 1**, lowest on purpose): every 500 ms -> `watchdog.pet()`. Lowest priority IS
   the safety: a hung higher task starves it -> no pet -> IWDG reboots.

Interrupt handlers (bottom of `main.cpp`): `SysTick_Handler` -> `HAL_IncTick()` + (once scheduler up)
`xPortSysTickHandler()`; `USART2_IRQHandler` -> `comms.onByteReceived()`; `DMA2_Stream0_IRQHandler` ->
`HAL_ADC_ConvCpltCallback` (the brain). PendSV/SVC handlers are the port's, named via FreeRTOSConfig
`#define`s.

## The mailBox queue (brain -> servoTask)

Replaced the old `volatile bool toggleRequested` flag. `xQueueCreate(4, sizeof(uint8_t))`. The brain
sends one token per flex (`xQueueSendFromISR`), servoTask receives (`xQueueReceive`). The queue does
its own locking, so no manual `volatile`/atomic reasoning. The token's *value* is unused, its
*existence* is the "a flex happened" signal (a counting semaphore would fit equally; the queue is the
more general tool).

## Where the "thinking" is

On the chip (`MuscleTrigger`): baseline track + center + notch + dip -> toggle, with a failsafe that
holds if the signal goes bad. The laptop is just a viewer of the `raw,centered,valid` stream
(`tools/emg_studio/chip_monitor.py`). The board runs standalone.

## On-chip detector knobs (`MuscleTrigger.h`)

| Knob | Value | Meaning |
|---|---|---|
| `DIP_THRESHOLD` | 425 | centered must dip this far below rest to count as a flex |
| `REARM_LEVEL` | 150 | ...and climb back above -this to re-arm for the next flex |
| `LOCKOUT` | 125 | samples (~125 ms at 1 kHz) after a release before another flex counts (bounce guard) |
| `RAIL_HIGH` / `RAIL_LOW` | 3000 / 25 | raw outside this band = bad signal (failsafe) |
| `VALID_AFTER` | 1000 | clean samples in a row (~1 s) needed to trust the signal again |

## Pins

| Part | Pin | Notes |
|---|---|---|
| EMG sensor | PA0 (A0) | ADC1 analog input |
| Servo | PB6 (D10) | TIM4 channel 1, PWM at 50 Hz |
| Serial TX / RX | PA2 / PA3 | USART2 over the ST-LINK virtual COM port |
| Onboard LED | PA5 (LD2) | used briefly for the A2 blink test, free now |
| CAN (not wired yet) | SPI2: PB13/PB14/PB15 + PB12 CS + PA9 INT | MCP2515 plan, see docs/wiring.md |
