# Firmware map

Navigation for `src/` + the FreeRTOS layout. The single place that says what each piece does, so the
files stay clean (no big header banners). One file = one part of the device.

Board: NUCLEO-F446RE. Chip: STM32F446RE. Language: C++ (header-only classes) + the FreeRTOS kernel in
`lib/`. Build: PlatformIO (`framework = stm32cube`), **hard-float** (see platformio.ini + fpu_link.py).
Flash: `pio run -t upload`.

## Files (in `src/`)

| File | What it is |
|---|---|
| `main.cpp` | Builds the parts + the two queues (`mailBox`, `canMailBox`), creates the 4 FreeRTOS tasks, calls `vTaskStartScheduler()`, and holds the interrupt handlers: SysTick (chained to HAL + kernel), the 1 kHz ADC/DMA brain callback, and the DMA stream IRQ. |
| `Emg.h` | `Emg` : the muscle sensor. TIM2 triggers the ADC on PA0 at 1 kHz and DMA parks each result in RAM; `read()` returns the freshest sample (no CPU per reading). |
| `MuscleTrigger.h` | `MuscleTrigger` : the on-chip brain. Tracks the resting baseline, centers the signal (through the notch), and returns `true` once per valid flex (a "dip"). Includes the signal-loss failsafe; `isElectrodeAttached()` for telemetry. |
| `Notch.h` | `Notch` : a 50 Hz biquad notch; `filter()` kills mains hum (used by MuscleTrigger). |
| `Servo.h` | `Servo` : the gripper. PWM on PB6 (TIM4 ch 1). `open()`/`close()`/`toggle()` choose a position; `ease()` glides toward it (slew-limited); `isClosed()` for telemetry. |
| `Comms.h` | `Comms` : one-way USB serial telemetry (USART2 TX). `sendStatus(raw, centered, valid)` streams the trace to the laptop viewer. |
| `Watchdog.h` | `Watchdog` : hardware IWDG (~2 s). `pet()` resets it; if nothing pets it, the chip reboots. |
| `Mcp2515CanBus.h` | `Mcp2515CanBus` : hand-written MCP2515 CAN driver (the logic). SPI2 bring-up, reset, register read/write, bit timing, mode select, `sendFrame()` / `readFrame()`. |
| `Mcp2515Registers.h` | The MCP2515 datasheet as code: every command byte, register address, and bit-field value, each with its datasheet name in the comment. |

## FreeRTOS layout

- `lib/FreeRTOS/` , the kernel (`tasks.c`/`list.c`/`queue.c`), the `ARM_CM4F` port (`port.c` +
  `portmacro.h`), `heap_4.c`, and the headers. Vendor code, copied from the stm32cube package, not edited.
- `include/FreeRTOSConfig.h` , OUR config (16 MHz HSI clock, 1 kHz tick, 10 KB heap, PendSV/SVC mapped
  to the port). In `include/` so both our code and the kernel see it; the vendor folder stays pure.
- Hard-float is mandatory for the CM4F port. The wiring is `platformio.ini` `build_flags` +
  `fpu_link.py` (the link half). Do NOT remove either, the build breaks (see the build note in the README).

## The 1 kHz brain + the four tasks

The brain runs in an **interrupt**, not a task. `HAL_ADC_ConvCpltCallback` fires at 1 kHz (every DMA
sample) and runs `trigger.update(emg.read())`: baseline + centering + notch + dip detection. On a valid
flex it `xQueueSendFromISR`s a token into BOTH `mailBox` (-> servoTask) and `canMailBox` (-> canTask),
then `portYIELD_FROM_ISR`s.

Four tasks (priorities: higher number = more urgent; idle is 0):
1. `servoTask` (**pri 3**, highest): `xQueueReceive(mailBox, ..., 5 ms)`. Token -> `servo.toggle()`;
   `servo.ease()` every cycle. The 5 ms receive-timeout doubles as the ~200 Hz pacing.
2. `commsTask` (**pri 2**): every 20 ms (~50 Hz) -> `comms.sendStatus(raw, centered, valid)`.
3. `canTask` (**pri 2**): `xQueueReceive(canMailBox, ..., 200 ms)`. A token -> immediate **gesture frame
   (0x100)**; a timeout -> **status heartbeat (0x101)**. Both carry `[gripper closed?, electrode attached?]`.
4. `watchdogTask` (**pri 1**, lowest on purpose): every 500 ms -> `watchdog.pet()`. Lowest priority IS
   the safety: a hung higher task starves it -> no pet -> IWDG reboots.

Interrupt handlers (bottom of `main.cpp`): `SysTick_Handler` -> `HAL_IncTick()` + (once scheduler up)
`xPortSysTickHandler()`; `HAL_ADC_ConvCpltCallback` = the brain; `DMA2_Stream0_IRQHandler` ->
`emg.handleDmaIrq()`. PendSV/SVC handlers are the port's, named via FreeRTOSConfig `#define`s.

## The two queues (brain -> tasks)

`mailBox` and `canMailBox`, each `xQueueCreate(4, sizeof(uint8_t))`. The brain drops one token per flex
into **both**: servoTask acts (toggle), canTask announces (CAN gesture frame). The queues do their own
locking, so no manual `volatile`/atomic reasoning. The token's *value* is unused; its *existence* is the
"a flex happened" signal. `canMailBox` is created before `mailBox`, so the `mailBox != NULL` ISR guard
(queues are NULL until the scheduler is set up) covers both.

## CAN driver (`Mcp2515CanBus.h` + `Mcp2515Registers.h`)

Hand-written from the datasheet (no mature STM32 MCP2515 library). SPI2 (PB13/14/15 + PB12 CS), 8 MHz
crystal @ 500 kbps, Normal mode. The board broadcasts a gesture frame (0x100) and a status heartbeat
(0x101); a second node (`bench/arduino_can_node/`) receives and decodes them. See README "The CAN work"
and `docs/wiring.md`.

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
| Serial TX | PA2 | USART2 TX over the ST-LINK virtual COM port (RX unused, telemetry is one-way) |
| CAN (MCP2515) | SPI2: PB13 SCK / PB14 MISO / PB15 MOSI + PB12 CS + PA9 INT | 8 MHz xtal @ 500 kbps, see docs/wiring.md |

(Mechanical/CAD assets live in `cad/` + `docs/`; see the README "Mechanical" section.)
