# STM32-EMG-Biosignal , HANDOFF (as of 2026-06-14)

Read this when continuing in a fresh chat. Repo: CanGitArchive/STM32-EMG-Biosignal at
`S:\Coding\STM32-EMG-Biosignal`. Owner: Can KADILAR. Second hardware portfolio piece (after the EMG
prosthetic-hand thesis): a myoelectric gripper on real STM32 firmware. **Going PUBLIC for the portfolio.**

> READ FIRST , how to work with Can: `S:\Coding\CodingPlaybooks\CODING_STYLE_PLAYBOOK.md`
> (ONE STEP AT A TIME: propose -> explain plainly -> get his go -> implement -> verify on hardware;
> NO autonomous multi-agent / ultracode). GOAL + history:
> `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_project_goals.md` (latest = Handoff 8)
> + `STM32_Project_Scope.md`. File-by-file nav: `FIRMWARE_MAP.md`.

## AUDIT FIRST (a rule earned the hard way)

Trust nothing in this file until you check it against `git log` + the actual `src/` files. Past handoffs
corrupted the plan by trusting prose over code. Verify any "done" claim against the code.

## STATE: COMPLETE , standalone gripper, on-chip + FreeRTOS + CAN + safety, VERIFIED (2026-06-14)

Every target competency is demonstrated on real hardware, in one running device:
- **Acquisition is hardware** (`Emg.h`): TIM2 -> ADC -> DMA at 1 kHz, no CPU per sample.
- **The 1 kHz brain** (`MuscleTrigger.h` + `Notch.h`) runs in `HAL_ADC_ConvCpltCallback`: baseline +
  50 Hz notch + centering + dip detection + signal-loss failsafe. On a flex it drops a token into BOTH
  `mailBox` and `canMailBox`.
- **Four FreeRTOS tasks** (`main.cpp`): `servoTask` (pri 3, toggle), `commsTask` (pri 2, serial
  telemetry), `canTask` (pri 2, CAN gesture 0x100 + status heartbeat 0x101), `watchdogTask` (pri 1,
  lowest = watchdog-starvation reboot).
- **CAN** is a hand-written MCP2515-over-SPI driver (`Mcp2515CanBus.h` + `Mcp2515Registers.h`), verified
  two-node against the Arduino node in `bench/arduino_can_node/`.

VERIFIED on hardware: a flex toggles the gripper and fires a `GESTURE` frame; idle sends `Status`
heartbeats; pulling an electrode -> gripper holds + `electrode=DETACHED` on CAN; no reboot loop.

## CRITICAL build note: hard-float wiring is load-bearing, do NOT "simplify" it

The ARM_CM4F FreeRTOS port emits FPU context-switch instructions, so the ENTIRE image must be hard-float.
On this ststm32 platform that takes BOTH halves: `platformio.ini` `build_flags`
(`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`) compile everything hard, AND `fpu_link.py` (a `post:` script)
forces the LINK hard (build_flags do NOT reach the ststm32 link step). Delete or "tidy" either one and
the link dies with "uses VFP register arguments, firmware.elf does not". This cost 4 failed attempts to map.

## FreeRTOS layout

- `lib/FreeRTOS/`: the kernel + `ARM_CM4F` port + `heap_4.c` + headers, copied from the stm32cube package
  (vendor, not edited).
- `include/FreeRTOSConfig.h`: OUR config (16 MHz HSI, 1 kHz tick, 10 KB heap,
  `configMAX_SYSCALL_INTERRUPT_PRIORITY` = 5, PendSV/SVC mapped to the port). In `include/` so the vendor
  folder stays pure.

## Hardware / wiring

- Nucleo-F446RE (Cortex-M4F), Mini-USB (not micro), HSI 16 MHz (no SystemClock_Config, deliberate).
  pio CLI: `~/.platformio/penv/Scripts/pio.exe` (NOT on PATH; call it with the PowerShell `&` operator).
- EMG: Grove EMG Detector analog out -> PA0 / A0 (ADC1).
- Servo: SG90 on TIM4_CH1 / PB6. V+ -> separate 6 V pack, GND -> battery AND Nucleo GND (shared ground
  mandatory). Measured: open 1300 us, close 1600 us.
- CAN: MCP2515 on SPI2 (PB13 SCK / PB14 MISO / PB15 MOSI / PB12 CS / PA9 INT), 8 MHz crystal @ 500 kbps.
  **Bus needs ~120 ohm termination across CANH-CANL**; unterminated -> occasional auto-retransmit
  duplicates (CAN reliability working). See `docs/wiring.md`.

## CAN: DONE, integrated, verified two-node

- Hand-written driver (no mature STM32 MCP2515 lib). Brought up in 4 verified steps: SPI reach the chip
  (CANSTAT = 0x80) -> bit timing + Normal mode -> loopback frame -> two-node transmit.
- Second node: `bench/arduino_can_node/` (Arduino UNO + MCP2515, coryjfowler MCP_CAN). `receiver.cpp`
  decodes the 0x100 (gesture) / 0x101 (status) frames to plain text.
- Captures: `docs/Logic_Analyzer_CAN_STM32_Comms_1..4.png` (STM32 SPI decode),
  `docs/Logic_Analyzer_CAN_Comms.png` (Arduino bench).

## Mechanical / CAD (portfolio assets)

Gripper designed in SolidWorks. `docs/gripper_cad.png` (render), `docs/gripper_animation.gif` (open/close
motion), `cad/gripper_assembly.stl` (GitHub 3D viewer) + `cad/print_parts/*.stl`. Gear math in
`docs/gripper_mechanics.md`; the gear-design tool in `tools/gear_designer/`. CAD source folders are
recorded in `...\CareerAssistant\career_information_navigation.md`.

## Laptop tools (`tools/emg_studio/`)

- `chip_monitor.py` , the viewer for the on-chip stream. Run with **Python 3.12**:
  `python tools\emg_studio\chip_monitor.py --port COM6`. (`operate.py`/`emg_studio.py`/`calibrate.py`
  predate the on-chip port.)

## Build / flash / CI

- Build: `& "C:\Users\kadil\.platformio\penv\Scripts\pio.exe" run -d "S:\Coding\STM32-EMG-Biosignal"`.
  Flash: add `-t upload` (ST-LINK over Mini-USB). CI builds `pio run` on every push.
- Commit: `git commit -m "subject" -m "body"` (NOT a piped here-string , it once injected a BOM into a
  commit title). Commit only when Can asks.

## DONE vs OPEN

**DONE + on hardware:** STM32/HAL, RTOS (4 tasks + 2 queues + watchdog-starvation safety), on-chip dip
classifier, servo control, DSP (50 Hz notch), timer+DMA sensing, signal-loss failsafe + IWDG, **CAN
(hand-written driver, two-node, integrated as gesture + heartbeat)**, Git/CI, README, demo video, CAD/STL,
BOM.

**OPEN (shipping tail only):**
- Flip the repo **PUBLIC** (it's clean: no secrets, good history).
- In the GitHub web editor, **drag `docs/demo_video_STM32.mp4` into the README hero spot** (marked with
  an HTML comment) for an inline player. `docs/gripper_animation.gif` already embeds.
- Feature on `canarchive.com`; add `STM32 / FreeRTOS / CAN` to the CV's embedded line.
- Optional: a GIF hero of the demo; CMSIS-DSP library swap; RX + hardware ID filter (deliberately skipped,
  off-narrative for a TX-only sensor/actuator node, the choice is documented in the README limitations).

## Reference docs in this repo

`FIRMWARE_MAP.md` (src nav) · `docs/wiring.md` · `docs/BOM.md` · `docs/gripper_mechanics.md` ·
`docs/lessons_learned.md` · `docs/emg_noise_findings.md` · `bench/arduino_can_node/README.md`.
