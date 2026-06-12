# Paste this whole file into the next chat (STM32 EMG project, chat 8)

You are continuing Can Kadilar's STM32 EMG-gripper project. STOP and read the files below BEFORE
writing or running anything. There are ~200 unrelated "handoff"/"HANDOFF.md" files across this coding
space, so use the EXACT ABSOLUTE PATHS below, never search by filename.

## RULE #1 , HOW TO WORK WITH CAN (read this first)

Go ONE STEP AT A TIME: propose -> explain in plain language -> discuss -> get his "go" -> implement ->
verify on the real hardware -> next step. Do NOT run ahead, do NOT build several subsystems before he
weighs in, and do NOT use autonomous multi-agent / "ultracode" on his code. (At the start of the 5th
session an "ultracode" run autonomously built a FreeRTOS port; it had to be rolled back entirely and
nearly ended the session. Do not repeat it.)

Can is a 15-year self-taught polyglot (C, C++, C#, Java, Python, PIC, Unreal/Unity/Godot). He is NOT a
beginner at programming logic , don't condescend. He IS often new to STM32 HAL / bare-metal jargon ,
decode acronyms plainly the first time (RX/TX, HAL, PWM, ADC, ISR, DMA, FPU, RTOS, SPI, MOSI/MISO).
He learns by discussion and edits the code himself; slower-but-understood beats fast-but-opaque every
time. He bridges well to game-engine / UE5 / Godot analogies. He will ask the SAME thing several ways
until it clicks, that's his method, not a problem, answer each angle fresh. Style: `while(1)` over
`for(;;)`; camelCase; explicit names; no big AI banner comments (one-line top comment max + the single
FIRMWARE_MAP.md nav doc). NO em-dashes or en-dashes in anything Can-facing (commas/colons/parens/hyphens).

## RULE #2 , AUDIT BEFORE YOU TRUST (this is why past handoffs kept breaking)

Earlier chats corrupted the plan by trusting the previous handoff instead of the code. Do not repeat:
- The FIRST chat is the source of truth: `STM32_project_goals.md` "Initial handoff" (the 0-9 step list)
  + the JD competency table in `STM32_Project_Scope.md`. Compare everything to THAT.
- Trust NO "done / settled" claim (including in THIS handoff) until you verify it against `git log` +
  the actual files. Read the code, not just the prose.
- Specific things to verify for chat 8:
  - FreeRTOS is genuinely DONE: commits a537fc2 / 400899e / 54f421b, and `src/main.cpp` has three tasks
    + an `xQueueCreate(mailBox)` + `vTaskStartScheduler()`. (Earlier chats once wrongly recorded
    FreeRTOS as "killed", it was never killed; it is now actually built and on hardware.)
  - The hard-float build wiring is LOAD-BEARING, not clutter: `platformio.ini` `build_flags`
    (-mfpu/-mfloat-abi=hard) AND `fpu_link.py` (the link half). If you "simplify" either, the link dies
    ("uses VFP register arguments, firmware.elf does not"). 4 failed attempts mapped this; don't redo it.
  - **Do NOT conflate "CAN hardware validated" with "STM32 CAN done."** The two MCP2515 modules + the
    logic analyzer were proven on an Arduino bench (`S:\Coding\CAN_test`). The STM32 CAN firmware is NOT
    written yet, that's the open work.
- If you find a contradiction, FLAG it to Can; do not silently inherit it.

## The locations (read in this order)

1. GOAL , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_project_goals.md`
   Why we're doing this + the running handoff log. Read the LATEST "## Handoff 7" section last , most
   current. When YOU hand off, append "## Handoff 8 (date)"; do NOT edit earlier sections.
2. SCOPE , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_Project_Scope.md`
   Parts, phased plan, and the Able Innovations "Embedded Systems Developer" JD this reverse-engineers.
3. STYLE , `S:\Coding\CodingPlaybooks\CODING_STYLE_PLAYBOOK.md` , how Can codes + how to pace a session
   (incl. the "teaching abstract systems concepts" notes added 2026-06-11).
4. BUILD STATE , `S:\Coding\STM32-EMG-Biosignal\HANDOFF.md` , current firmware/architecture/tools.
5. NAV , `S:\Coding\STM32-EMG-Biosignal\FIRMWARE_MAP.md` , each src file + FreeRTOS task/queue layout + knobs.
6. WIRING , `S:\Coding\STM32-EMG-Biosignal\docs\wiring.md` , pin plan incl. the MCP2515/SPI2 CAN wiring.
7. REFERENCE ONLY , `S:\Coding\PublicRepos\Prosthetic-Hand-Single-EMG-Multi-Pattern\README.md` , Can's
   published M.Sc. thesis (the prior work this rebuilds). Do not edit.

## The repo

`S:\Coding\STM32-EMG-Biosignal` (CanGitArchive/STM32-EMG-Biosignal, private, `gh` authed). Firmware =
`src/` (C++ header-only classes) + `lib/FreeRTOS/` (kernel). Config = `include/FreeRTOSConfig.h`,
`platformio.ini`, `fpu_link.py`. Laptop tools = `tools/emg_studio/`. Latest commit on main: 54f421b
(VERIFY with `git log`; do not trust this hash blindly).

## State in one paragraph (VERIFY against the code)

Standalone on-chip myoelectric gripper on real STM32 firmware, now running under FreeRTOS. Acquisition
is hardware (TIM2 -> ADC -> DMA at 1 kHz, `Emg.h`). The brain (baseline + centering + 50 Hz notch + dip
detection, `MuscleTrigger.h` + `Notch.h`) runs at 1 kHz in `HAL_ADC_ConvCpltCallback`; on a flex it
`xQueueSendFromISR`s a token into the `mailBox` queue + `portYIELD_FROM_ISR`s. Three tasks: `servoTask`
(pri 3, blocks on the queue w/ a 5 ms timeout that also paces it, toggles + eases), `commsTask` (pri 2,
~50 Hz telemetry), `watchdogTask` (pri 1 = lowest, pets the IWDG, so a hung higher task -> reboot).
SysTick feeds both HAL and the kernel. VERIFIED on hardware. CAN hardware (2x MCP2515 + analyzer)
validated on an Arduino bench; STM32 CAN code not written yet.

## DONE (verify) vs OPEN

DONE + committed + on hardware: toolchain, part bring-up, on-chip dip classifier, servo control, 50 Hz
notch (DSP), timer+DMA sensing, signal-loss failsafe + IWDG, **FreeRTOS (3 tasks + mailBox queue +
watchdog-starvation reboot)**, Git/CI. CAN **hardware** validated on the bench (`S:\Coding\CAN_test`).
OPEN: **CAN STM32 firmware** (the code, NEXT), Ship (README + demo video + CV line). CMSIS-DSP library
swap optional (the notch already shows the DSP box).

## SETTLED decisions , do NOT re-open

- ONE gesture only: a single dip = toggle open/close. No DTW on the chip (the thesis owns DTW).
- C++ header-only classes (one `main.cpp` + a class per header). No big AI banner comments.
- FreeRTOS is DONE and KEPT (3 tasks + queue). The working super-loop fallback is in git history (71af159).
- The hard-float build wiring (platformio.ini build_flags + fpu_link.py) is required; do not remove.

## NEXT ACTION , STM32 CAN firmware (the last JD box). CAN hardware is already validated.

Write a minimal MCP2515-over-SPI driver on the F446 and broadcast the gesture/telemetry as CAN frames,
then decode them on the logic analyzer (same PulseView workflow). The Arduino bench already proved the
two modules, the 8 MHz crystal, 500 kbps, and the analyzer, so ANY failure here is firmware, not hardware.
- Wiring plan (docs/wiring.md): MCP2515 on **SPI2** , PB13 SCK / PB14 MISO / PB15 MOSI / PB12 CS /
  PA9 INT. 5 V + GND to the module.
- There is NO mature STM32 MCP2515 HAL library. Port/write a small register-level driver (init, set
  bitrate for an 8 MHz crystal @ 500 kbps to match the bench, send a frame). Keep it in Can's
  header-only-class style (e.g. a `Can.h`). One step at a time; he'll want to understand the SPI
  register dance.
- Then a `canTask` (or fold into commsTask) broadcasts the detected gesture / telemetry. Decode the
  STM32's frames on the analyzer to confirm (the bench `.sr`/`.pvs`/screenshot in `docs/` show the
  target).
- Reuse the bench: `S:\Coding\CAN_test` (Arduino) is a known-good second CAN node if you want a real
  STM32 -> Arduino send/receive exchange, not just an analyzer decode.

After CAN: Ship , README (scope + results + wiring + the docs/Logic_Analyzer_CAN_Comms.png), short demo
video, add `STM32 / FreeRTOS / CAN` to the CV's embedded line.

## Build / flash / verify

- Build: `& "C:\Users\kadil\.platformio\penv\Scripts\pio.exe" run -d "S:\Coding\STM32-EMG-Biosignal"`
  (pio is NOT on PATH; use the PowerShell `&` operator). Flash: add `-t upload` (ST-LINK over Mini-USB).
- Verify on hardware: flash, then `python tools\emg_studio\chip_monitor.py --port COM6` , run it with
  **Python 3.12** (it has numpy/pyserial/PyQt6/pyqtgraph; the auto-installed Python 3.14 does not).
- Logic analyzer: it's an fx2lafw clone needing a one-time Zadig WinUSB swap (zadig.exe at
  `C:\Program Files (x86)\sigrok\PulseView\`); then PulseView sees it. Already done this session.
- Commit: `git commit -m "subject" -m "body"` (NOT a piped here-string , it once injected a BOM into a
  commit title). Commit only when Can asks.

## HARD RULES

- No em-dashes or en-dashes in any Can-facing or public text (commas, colons, parentheses, hyphens).
- One step at a time; no autonomous multi-agent / ultracode (see Rule #1).
- When you hand off: append "## Handoff 8 (date)" to `STM32_project_goals.md` (do not edit earlier
  sections), refresh THIS file + `HANDOFF.md` + `FIRMWARE_MAP.md`, and keep the AUDIT section (Rule #2)
  so chat 9 starts skeptical too.
