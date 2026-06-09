# Paste this whole file into the next chat (STM32 EMG project, chat 6)

You are continuing Can Kadilar's STM32 EMG-gripper project. STOP and read the files below BEFORE
writing or running anything. There are ~200 unrelated "handoff"/"HANDOFF.md" files across this
coding space, so use these EXACT ABSOLUTE PATHS, never search by filename.

## HOW TO WORK WITH CAN (read this FIRST , it is the #1 rule)

Go ONE STEP AT A TIME. For each step: propose it -> explain it in plain language -> discuss ->
get his "go" -> implement -> verify on the real hardware -> next step. Do NOT run ahead, do NOT
build several subsystems before he weighs in, do NOT "finish the whole thing" autonomously, and do
NOT use autonomous multi-agent / "ultracode" orchestration on his code.

(Why this is rule #1: at the start of the 5th session (2026-06-09, NOT day one of the project) an
"ultracode" run autonomously built an entire FreeRTOS port before Can could weigh in. It had to be
rolled back completely and nearly ended the session. Don't repeat it. Note: the rollback was about
the AUTONOMOUS method, NOT a decision to drop FreeRTOS , see "Known deviations" below.)

Can is a 15-year self-taught polyglot (C, C++, C#, Java, Python, PIC, Unreal/Unity/Godot). He is
NOT a beginner at programming logic , don't condescend. He IS often new to a specific domain's
jargon (STM32 HAL, bare-metal embedded) , decode acronyms plainly the first time (RX/TX, HAL, PWM,
ADC, ISR). He learns by discussion and edits the code himself. Full guidance is in the style
playbook (location 3 below) , read it before doing anything.

## The locations (read in this order) , and what each one IS

1. GOAL , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_project_goals.md`
   The "why are we doing this" anchor + the running handoff log. Read the LATEST "## Handoff 5"
   section last , it is the most current. When YOU hand off, append a new "## Handoff N (date)"
   here (do not edit earlier sections).
2. SCOPE , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_Project_Scope.md`
   Parts list, phased plan, and the mapping to the Able Innovations "Embedded Systems Developer" JD
   that this project is reverse-engineered from.
3. STYLE , `S:\Coding\CodingPlaybooks\CODING_STYLE_PLAYBOOK.md`
   How Can codes and how to pace a session with him. This is the cross-project hub
   (`S:\Coding\CodingPlaybooks\`); `CODING_CONVENTIONS.md` next to it is for his Python apps, not
   this firmware. READ THE STYLE DOC before touching code.
4. BUILD STATE , `S:\Coding\STM32-EMG-Biosignal\HANDOFF.md`
   The current firmware/architecture/tools state (this repo's build handoff).
5. NAV , `S:\Coding\STM32-EMG-Biosignal\FIRMWARE_MAP.md`
   What each source file in `src/` is, plus the detector's tunable knobs.
6. REFERENCE ONLY , `S:\Coding\PublicRepos\Prosthetic-Hand-Single-EMG-Multi-Pattern\README.md`
   Can's published M.Sc. thesis (the prior work this rebuilds, and where his C++ header-only-class
   style comes from). Do not edit it.

## The repo

`S:\Coding\STM32-EMG-Biosignal` (CanGitArchive/STM32-EMG-Biosignal, private, `gh` authed). Firmware
= `src/` (C++ header-only classes). Laptop tools = `tools/emg_studio/`. This is a SEPARATE repo
under `S:\Coding`; it is not the folder a chat may open by default, so point yourself at it.
Ignore: `C:\Users\kadil\.platformio\...` (only touched during the rolled-back ultracode recon) and
any `*_check.py` in the temp folder (throwaway).

## State in one paragraph

The board now does EVERYTHING on-chip and runs standalone. Firmware is a clean C++ super-loop
(the CURRENT implementation, not an RTOS; FreeRTOS is still open, see "Known deviations") of
header-only classes: Emg (ADC/PA0), MuscleTrigger (live baseline +
centering + dip detection + signal-loss failsafe), Servo (PB6 PWM; open/close/toggle/ease), Comms
(USART2; streams "raw,centered,valid"), Timer (200 Hz metronome). A flex dips the centered signal
past -425 and toggles the gripper (one toggle per flex; bounce-guard + re-arm); pulling an
electrode rails the signal, which the failsafe catches (gripper holds, baseline frozen, re-trusts
after ~1 s). VERIFIED on hardware (fast wrist-flicks toggle cleanly, unplug holds, stable). The
laptop is just a VIEWER now: `tools/emg_studio/chip_monitor.py` (live centered graph + threshold
lines + valid banner + decimated CSV logging to logs/). Latest commit: see `git log`.

## SETTLED decisions , do NOT re-open

- ONE gesture only: a single dip = toggle open/close. No DTW on the chip (the thesis owns DTW), no
  dip-counter (dropped).
- C++ header-only classes (one main.cpp + a class per header, Can's thesis style). Not the C .h/.c split.
- No big AI file-header banner comments. Inline comments + the single FIRMWARE_MAP.md nav doc.

## Known deviations from the original scope (NOT approved cuts , flagged 2026-06-09 audit)

The first chat's plan + the Able JD table list these; they are NOT done. An earlier handoff wrongly
recorded the DSP as "done" and FreeRTOS as a settled cut. Open work, not closed decisions:
- FreeRTOS / RTOS , current loop is a super-loop; the sample/process/comms-tasks deliverable is open.
- DMA + timer-triggered ADC , the ADC is polled once per loop (inherited from the streamer).
- CMSIS-DSP band-pass / features , on-chip processing is only a lightweight EMA baseline + centering.

## NEXT ACTION (Can picks; do not assume; one step at a time)

The core on-chip DECISION is done and verified, but the competencies above remain open. Remaining
work, highest value first:
- CAN (MCP2515 over SPI): broadcast the gesture/telemetry on an industrial bus. Biggest remaining JD
  competency + best demo visual (logic-analyzer CAN decode). Needs the MCP2515 module wired.
- IWDG watchdog: reboots the chip if the code hangs (quick; complements the signal-loss failsafe).
- Ship: README (turn a chip_log CSV into a clean graph) + demo video + add the STM32/CAN line to the CV.
Ask Can which one.

## Build / flash / verify

- Build: `& "C:\Users\kadil\.platformio\penv\Scripts\pio.exe" run -d "S:\Coding\STM32-EMG-Biosignal"`
  (pio is NOT on PATH). Flash: add `-t upload` (ST-LINK over Mini-USB).
- Verify on hardware (Can has it wired): flash, then `python tools/emg_studio/chip_monitor.py --port COM6`
  to watch + log. Close other COM users first.

## HARD RULES

- No em-dashes or en-dashes in any Can-facing or public text (use commas, colons, parentheses, hyphens).
- One step at a time; no autonomous multi-agent / ultracode (see the top).
- When you hand off, append a "## Handoff N (date)" to STM32_project_goals.md (do not edit earlier
  sections) and refresh this file + HANDOFF.md.
