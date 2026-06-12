# STM32-EMG-Biosignal , HANDOFF (as of 2026-06-12)

Read this when continuing in a fresh chat. Repo: CanGitArchive/STM32-EMG-Biosignal (PRIVATE) at
`S:\Coding\STM32-EMG-Biosignal`. Owner: Can Kadilar. Second hardware portfolio piece (after the EMG
prosthetic-hand thesis): a myoelectric gripper on real STM32 firmware. Edge-AI / embedded.

> GOAL is not in this file. Read first:
> `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_project_goals.md` (the why,
> latest section = Handoff 7) + `STM32_Project_Scope.md` (parts, plan, JD-mapping). And read
> `S:\Coding\CodingPlaybooks\CODING_STYLE_PLAYBOOK.md` (how to work with Can: ONE STEP AT A TIME,
> no autonomous multi-agent / ultracode). File-by-file nav: `FIRMWARE_MAP.md`.

## STATE: on-chip + standalone + under FreeRTOS, VERIFIED (2026-06-12)

The board reads the muscle, makes the open/close decision ON-CHIP, drives the gripper, fails safe, and
reboots itself if a task hangs, all standalone, and now all running under **FreeRTOS** (real tasks +
a queue, not a super-loop). The laptop is just a viewer/logger.

Firmware = header-only classes in `src/` + the FreeRTOS kernel in `lib/FreeRTOS/`. Architecture:
- **Acquisition is hardware** (`Emg.h`): TIM2 triggers the ADC on PA0 at 1 kHz; DMA parks each result
  in RAM; `read()` returns the freshest sample (no CPU per reading).
- **The 1 kHz brain** (`MuscleTrigger.h` + `Notch.h`) runs inside `HAL_ADC_ConvCpltCallback` (fired by
  the DMA-complete interrupt): live baseline EMA + centering through a 50 Hz notch + dip detection +
  signal-loss failsafe. On a valid flex it does NOT set a flag anymore, it `xQueueSendFromISR`s a token
  into the `mailBox` queue and `portYIELD_FROM_ISR`s so the (higher-priority) servoTask runs at once.
- **Three FreeRTOS tasks** (`main.cpp`):
  - `servoTask` (priority 3, highest): blocks on `xQueueReceive(mailBox, ..., 5 ms)`. On a token it
    `servo.toggle()`s; every cycle it `servo.ease()`s. The 5 ms receive-timeout doubles as the ~200 Hz
    pacing (it replaced the old `vTaskDelay`/`Timer`).
  - `commsTask` (priority 2): every 20 ms (~50 Hz) reads `emg.read()` + `trigger.centered()` and
    `comms.sendStatus(raw, centered, valid)`.
  - `watchdogTask` (priority 1, LOWEST on purpose): every 500 ms `watchdog.pet()`. Lowest priority IS
    the safety: a hung higher task starves this -> no pet -> IWDG reboots the chip.
- **Interrupt handlers** (bottom of `main.cpp`): `SysTick_Handler` calls BOTH `HAL_IncTick()` and (once
  the scheduler is up) `xPortSysTickHandler()`. `USART2_IRQHandler` -> `comms.onByteReceived()` (the
  old `S<us>` receive path, now unused, kept as reference). `HAL_ADC_ConvCpltCallback` = the brain.
  `DMA2_Stream0_IRQHandler` -> `emg.handleDmaIrq()`. PendSV/SVC handlers come from the ARM_CM4F port,
  wired by `#define`s in `include/FreeRTOSConfig.h`.
- The part classes (`Emg`/`Servo`/`Comms`/`MuscleTrigger`/`Watchdog`/`Notch`) are unchanged from the
  super-loop era. `Timer.h` is no longer used (vTaskDelay + the queue timeout replaced it); harmless to
  leave, candidate for a cleanup pass.

VERIFIED on hardware (2026-06-12): a flex toggles the gripper one-per-flex including fast wrist-flicks;
pulling an electrode -> gripper holds, reattach recovers ~1 s; no reboot loop. Detector tunables are
named consts at the bottom of MuscleTrigger.h.

## CRITICAL build note: hard-float wiring is load-bearing, do NOT "simplify" it

The ARM_CM4F FreeRTOS port (`port.c`) emits FPU context-switch instructions, so the ENTIRE image must
be hard-float. On this ststm32 platform that takes BOTH halves, and both are documented in the files:
- `platformio.ini` `build_flags`: `-mfpu=fpv4-sp-d16 -mfloat-abi=hard` (compiles framework + src + lib
  hard).
- `fpu_link.py` (a `post:` extra_script): adds the same flags to the LINK (PlatformIO's `build_flags`
  do NOT reach the ststm32 link step; without this the linker pulls the soft-float crt0/libc and the
  link dies with "uses VFP register arguments, firmware.elf does not").
Delete or "tidy" either one and the build breaks. This cost 4 failed link attempts to map; don't redo
that.

## FreeRTOS layout

- `lib/FreeRTOS/`: the kernel (`tasks.c`, `list.c`, `queue.c`), the `ARM_CM4F` `port.c` + `portmacro.h`,
  `heap_4.c`, and all the headers. Copied from the stm32cube package; we do not edit vendor files.
- `include/FreeRTOSConfig.h`: OUR config (16 MHz HSI `configCPU_CLOCK_HZ`, 1 kHz tick, heap 10 KB,
  `configMAX_SYSCALL_INTERRUPT_PRIORITY` = 5, PendSV/SVC mapped to the port). It lives in `include/`
  (on the path for both our code and the kernel), NOT in `lib/FreeRTOS/`, so the vendor folder stays
  pure.

## Hardware / wiring

- Nucleo-F446RE (Cortex-M4F). Powers/flashes over Mini-USB (not micro). PlatformIO
  (`framework = stm32cube`, board `nucleo_f446re`). pio CLI: `~/.platformio/penv/Scripts/pio.exe`
  (NOT on PATH; call it with the PowerShell `&` operator). Clock = HSI 16 MHz (no SystemClock_Config;
  deliberate).
- EMG: Grove EMG Detector analog out -> PA0 / A0 (ADC1), powered from 3V3.
- Servo: SG90 on TIM4_CH1 / PB6 (D10). V+ -> separate 6 V pack, GND -> battery AND Nucleo GND (shared
  ground mandatory). Measured: open 1300 us, close 1600 us.
- CAN (STM32 side NOT built yet): MCP2515 over SPI2, planned PB13 SCK / PB14 MISO / PB15 MOSI /
  PB12 CS / PA9 INT , see `docs/wiring.md`.

## CAN hardware: VALIDATED on the Arduino bench (2026-06-12), STM32 code not written yet

Before writing any STM32 CAN, the two MCP2515 modules + the logic analyzer were proven on Arduino (the
"Turkey faulty-part" gate). Bench project lives OUTSIDE this repo at `S:\Coding\CAN_test` (PlatformIO,
`atmelavr`/`uno`, two envs `sender`/`receiver` via `build_src_filter`, lib = coryjfowler MCP_CAN):
- Two UNOs + two MCP2515 modules (8 MHz crystals, 500 kbps). Verified BOTH directions (each module did
  TX and RX), frames correct (id 0x100, `45 4D 47` + counter).
- Logic analyzer = an fx2lafw "Saleae Logic" clone. PulseView installed via winget (`Sigrok.PulseView`).
  It needs a one-time **Zadig** WinUSB driver swap before PulseView sees it (`zadig.exe` is at
  `C:\Program Files (x86)\sigrok\PulseView\`). Confirmed: captured + SPI-decoded the frame. Screenshot:
  `docs/Logic_Analyzer_CAN_Comms.png`.
- CAN gotcha learned: a CAN transmit only succeeds if ANOTHER node ACKs it, so "Send FAILED err 6/7" =
  the CANH/CANL bus isn't connected (wire CANH-CANH and CANL-CANL, straight, not crossed).

## Laptop tools (tools/emg_studio/, PyQt6 + pyqtgraph)

- `chip_monitor.py` , THE viewer for the on-chip stream (live centered graph + -425/-150 lines +
  SIGNAL OK/LOST banner + decimated CSV logging). Run with **Python 3.12** (it has the deps; see
  `tools/emg_studio/requirements.txt`): `python tools\emg_studio\chip_monitor.py --port COM6` (close
  other COM users first). Defaults now `--fs 50` (matches commsTask) and `--seconds 5`.
- `operate.py` / `emg_studio.py` / `calibrate.py` predate the on-chip port; chip_monitor.py is the one
  that matches the current firmware.

## Build / flash / CI

- Build: `& "C:\Users\kadil\.platformio\penv\Scripts\pio.exe" run -d "S:\Coding\STM32-EMG-Biosignal"`.
  Flash: add `-t upload` (ST-LINK over Mini-USB). CI builds `pio run` on every push.
- Milestone commits (verify with `git log`, do not assume "latest"): a537fc2 (FreeRTOS A1+A2 kernel +
  blink), 400899e (B2, three tasks), 54f421b (Step C priorities + queue + viewer fix). All on
  origin/main.
- Commit: `git commit -m "subject" -m "body"` (NOT a piped here-string , it once injected a BOM into a
  commit title). Commit only when Can asks.

## DONE vs OPEN (against the Able JD)

DONE + on hardware: STM32/HAL, **RTOS (tasks + queue + watchdog-starvation safety)**, on-chip dip
classifier, servo control, DSP (50 Hz notch), timer+DMA sensing, signal-loss failsafe + IWDG, Git/CI.
CAN **hardware** validated on the bench. OPEN: **CAN STM32 firmware** (the code, not yet written), Ship
(README + demo video + CV line). CMSIS-DSP library = optional (the notch already shows the DSP box).

## NEXT (Can picks, one step at a time)

- **STM32 CAN** (the last JD box): write a minimal MCP2515-over-SPI driver on SPI2 (no mature HAL lib
  exists, port a register-level one), broadcast the gesture/telemetry, decode the STM32's frames on the
  analyzer with the same PulseView SPI workflow. Hardware is known-good, so any failure = firmware.
- **Ship**: README (scope + results + wiring + the logic-analyzer screenshot) + demo video + CV line
  (STM32 / FreeRTOS / CAN).

## Reference docs in this repo

- `FIRMWARE_MAP.md` , what each src file is, the FreeRTOS task/queue layout, and the detector knobs.
- `docs/wiring.md` , pin plan incl. the planned MCP2515/SPI2 wiring. `docs/lessons_learned.md` , the
  EMG control saga (read before touching the detector). `docs/gripper_mechanics.md`, `docs/emg_noise_findings.md`.
- `docs/Logic_Analyzer_CAN_Comms.png` , the validated CAN-over-SPI capture (portfolio asset).
