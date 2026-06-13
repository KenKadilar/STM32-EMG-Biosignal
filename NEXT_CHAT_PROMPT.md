# Paste this whole file into the next chat (STM32 EMG project, chat 9)

You are continuing Can KADILAR's STM32 EMG-gripper project. **The engineering is DONE and verified on
hardware.** What remains is the shipping tail (make public, portfolio, CV). STOP and read the files below
before writing or running anything. Use the EXACT ABSOLUTE PATHS below; never search by filename
(there are ~200 unrelated "handoff" files across this coding space).

## RULE #1 , HOW TO WORK WITH CAN (read this first)

Go ONE STEP AT A TIME: propose -> explain in plain language -> discuss -> get his "go" -> implement ->
verify on the real hardware -> next step. Do NOT run ahead, do NOT build several things before he weighs
in, and do NOT use autonomous multi-agent / "ultracode" on his code (a 5th-session ultracode run
autonomously built a FreeRTOS port and had to be rolled back entirely).

Can is a 15-year self-taught polyglot (C, C++, C#, Java, Python, PIC, Unreal/Unity/Godot). He is NOT a
beginner at programming logic , don't condescend. He IS often new to STM32 / embedded jargon , decode
acronyms plainly the first time. He learns by discussion and edits the code himself; slower-but-understood
beats fast-but-opaque. He will ask the SAME thing several ways until it clicks , answer each angle fresh.
He catches AI-tells (em-dashes, the `·` interpunct, "Not-A syndrome" negations that plant bad words like
"not Arduino") , keep public/Can-facing text clean: **no em-dashes or en-dashes**, no interpunct, no
hobbyist-planting negations. Read `S:\Coding\CodingPlaybooks\CODING_STYLE_PLAYBOOK.md` FIRST.

## RULE #2 , AUDIT BEFORE YOU TRUST

Earlier chats corrupted the plan by trusting the previous handoff instead of the code. Trust NO
"done/settled" claim (including in THIS file) until you verify it against `git log` + the actual files.
The FIRST chat is the source of truth: `STM32_project_goals.md` "Initial handoff" (the 0-9 steps). If you
find a contradiction, FLAG it to Can.

## The locations (read in this order)

1. GOAL + history , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_project_goals.md`
   (read the latest section, "## Handoff 8", last).
2. SCOPE , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\STM32_Project_Scope.md`.
3. STYLE , `S:\Coding\CodingPlaybooks\CODING_STYLE_PLAYBOOK.md`.
4. BUILD STATE , `S:\Coding\STM32-EMG-Biosignal\HANDOFF.md`.
5. NAV , `S:\Coding\STM32-EMG-Biosignal\FIRMWARE_MAP.md`.
6. CAREER INDEX , `S:\Coding\ChatAssistants\HealthAssistant\CareerAssistant\career_information_navigation.md`
   (CV, LinkedIn, publications, portfolio, CAD source folders, job log).

## The repo

`S:\Coding\STM32-EMG-Biosignal` (CanGitArchive/STM32-EMG-Biosignal, `gh` authed). Firmware = `src/`
(C++ header-only classes) + `lib/FreeRTOS/`. CAN driver = `Mcp2515CanBus.h` + `Mcp2515Registers.h`.
Bench second node = `bench/arduino_can_node/`. CAD = `cad/` + `docs/`. (VERIFY commits with `git log`.)

## State in one paragraph (VERIFY against the code)

COMPLETE. Standalone myoelectric gripper on real STM32 firmware: TIM2 -> ADC -> DMA at 1 kHz; the 1 kHz
brain (baseline + 50 Hz notch + dip detection + failsafe) runs in `HAL_ADC_ConvCpltCallback` and drops a
token into BOTH `mailBox` and `canMailBox`. Four FreeRTOS tasks: servoTask (pri 3), commsTask (pri 2),
canTask (pri 2, CAN gesture 0x100 + status heartbeat 0x101), watchdogTask (pri 1 lowest = starvation
reboot). CAN is a hand-written MCP2515-over-SPI driver, verified two-node against the Arduino node. All
verified on hardware. Hard-float wiring (platformio.ini + fpu_link.py) is load-bearing, do not "simplify".

## DONE vs OPEN

DONE: every competency (STM32, RTOS, CAN, on-chip DSP + classification, control, safety, Git/CI) + README
+ demo video + CAD/STL + BOM. OPEN (shipping tail only): flip the repo PUBLIC; on GitHub, drag
`docs/demo_video_STM32.mp4` into the README hero (HTML-comment marker) for an inline player; feature on
`canarchive.com`; add `STM32 / FreeRTOS / CAN` to the CV embedded line. Optional: GIF hero; CMSIS-DSP swap;
RX + hardware ID filter (deliberately skipped, off-narrative, documented in README limitations).

## SETTLED decisions , do NOT re-open

- ONE gesture only: a single dip toggles open/close (the thesis owns multi-grip DTW).
- C++ header-only classes; no big AI banner comments.
- CAN is TX-only in the product (gesture + heartbeat); RX/filter deliberately skipped.
- Hard-float build wiring required; do not remove platformio.ini flags or fpu_link.py.

## Build / flash / verify

- Build: `& "C:\Users\kadil\.platformio\penv\Scripts\pio.exe" run -d "S:\Coding\STM32-EMG-Biosignal"`
  (pio NOT on PATH; use the PowerShell `&`). Flash: add `-t upload` (ST-LINK over Mini-USB).
- Viewer: `python tools\emg_studio\chip_monitor.py --port COM6` , Python 3.12. CAN second node:
  `bench/arduino_can_node/` (`pio run -e receiver -t upload`, monitor COM8).
- Commit: `git commit -m "subject" -m "body"` (NOT a piped here-string). Commit only when Can asks.

## HARD RULES

- No em-dashes / en-dashes / `·` interpunct, and no hobbyist-planting negations, in any public/Can-facing text.
- One step at a time; no autonomous multi-agent / ultracode.
- When you hand off: append "## Handoff 9 (date)" to `STM32_project_goals.md` (do not edit earlier
  sections), refresh THIS file + `HANDOFF.md` + `FIRMWARE_MAP.md`, keep the AUDIT section so chat 10 starts skeptical.
