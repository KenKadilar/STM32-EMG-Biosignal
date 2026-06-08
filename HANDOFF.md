# STM32-EMG-Biosignal , HANDOFF (as of 2026-06-08)

Read this first when continuing in a fresh chat. Repo: `CanGitArchive/STM32-EMG-Biosignal`
(PRIVATE) at `S:\Coding\STM32-EMG-Biosignal`. Owner: Can Kadilar. This is the second
hardware portfolio piece (after the EMG prosthetic-hand thesis): a myoelectric gripper
on real STM32 firmware. Edge-AI / embedded positioning.

## STATE: WORKING end to end (2026-06-08)

Muscle EMG -> on-device DSP -> DTW gesture classification -> slew-limited servo ->
3D-printed involute-gear gripper opens/closes. Confirmed working (log
`tools/emg_studio/dtw_log_260608-131705.csv`). Stage 1 (open/close via weak/strong pulse).

## Hardware / wiring

- Nucleo-F446RE (Cortex-M4F). Powers/flashes over **Mini-USB** (not micro). Driver:
  STSW-LINK009 (was needed for the Code 28). Toolchain = **PlatformIO** (`framework =
  stm32cube`, board `nucleo_f446re`). pio CLI: `~/.platformio/penv/Scripts/pio.exe` (NOT on PATH).
- EMG: Grove EMG Detector, analog envelope-ish out -> **PA0 / A0** (ADC1). Power Grove from
  **3V3** (PA0 is not 5V tolerant). Electrodes: red+black on palmaris longus, white ref on wrist.
- Servo: SG90 on **TIM4_CH1 / PB6 (D10)**. Signal->D10, V+->separate 6V pack, GND->battery-
  AND Nucleo GND (shared ground mandatory). Measured: **open 1300us, close 1600us**, clamp
  1270-1620, boots open. SG90 deadband ~1-2deg (small jogs may not register; full moves are fine).
- Gripper: 3D-printed involute-gear pincher. Servo gear -> idler -> 2 finger gears.

## Firmware (`src/main.c`)

Unified: streams raw ADC (PA0) over USART2 at ~200 Hz (one int/line) AND listens for
`S<us>\n` servo commands (RX interrupt). Drives the SG90 with slew-limiting (eases, never
slams) + safe clamp. The EMG DSP + DTW are currently done on the LAPTOP, not on-chip; the
firmware is a dumb streamer + servo. (Porting DTW onto the MCU is a future edge-AI step.)
Flash: `pio run -t upload`.

## Laptop tools (`tools/emg_studio/`, PyQt6 + pyqtgraph)

All GUIs ignore SIGINT (Can's mouse sends stray ones; close via window X). Close the
PlatformIO Serial Monitor before running any (one program per COM port).

- `emg_studio.py` , window 1: live signal + envelope + connection-health banner. Holds the
  SHARED CORE: `Notch`, `Ring` (lock-free), `SerialReader` (per-sample DSP, prime, record hooks).
- `calibrate.py` , window 2: record (relax/open/close) -> trim -> save as named/dated
  templates. Library table: search, filter, **active checkbox** (only active are used),
  **editable name + label** (relabel), **Edit/re-trim selected** (load a saved template back to
  re-trim/relabel and update in place). 3 comparison slots show active templates. Panel-height
  constants at top.
- `operate.py` , window 3: DTW match + decision FSM + drives the servo. Live tunables.
- `servo_test.py` , manual servo jog (open/close presets, safe band).
- `templates.json` , the template library (each: id/name/label/created/active/env).

## Signal + decision pipeline

Per-sample in `SerialReader`: 50 Hz notch biquad -> adaptive DC removal -> rectify ->
moving-average envelope -> rings. DTW (operate.py): per label, **min over active templates**,
live window = last N samples (N = template length) resampled to L=40, classic DTW.

Decision FSM: IDLE -> (env>activity) PULSE [track min-DTW per action] -> (env<off) DECIDE
(best action if min-DTW < cap AND beats runner-up by margin) -> REFRACTORY (lockout + env
back to rest) -> IDLE. 3 s warm-up at start. operate.py sends `S<open_us/close_us>` on
gripper change. Live tunables (defaults): activity env >40, **cap <1500**, **margin 1.3x**,
lockout 1.2 s.

## Gear tool (`tools/gear_designer/`)

`gear_designer.py` (CLI) + `gear_designer_gui.py` (GUI): involute spur-gear generator,
DXF export (for SolidWorks), live mesh preview, center-distance + undercut + bore-vs-root
warnings. Rules: one shared module, center = m(z1+z2)/2, addendum=m, >=14 teeth OR high
pressure angle (30deg lets you go to ~8 teeth). The inverted-tooth-flank bug is FIXED.

## Key learnings (don't relearn)

- EMG noise: 50 Hz mains (plugged), **contact-driven** not random; battery cleaner. Fault
  signatures: low DC = signal/power-side loss, high DC = reference loss, clip = railing.
  Full writeup + plots + data: `docs/emg_noise_findings.md` + `docs/data/`.
- **Templates must be CONSISTENT** length/type (both ~0.7-1 s pulses, differ in INTENSITY
  not length). Inconsistent (short weak open vs long strong close) caused the misfires; the
  Edit/re-trim feature exists to fix that.
- Don't AVERAGE DTW templates (smears the shape); keep individuals, min-match.
- Threshold control failed (adaptive-bias lag on relax + EMG fatigue droop). DTW
  pulse-based is the approach. Threshold version kept as documented baseline in git history.
- Let electrodes settle ~10 min before calibrating (impedance drift).

## OPEN / NEXT items

1. **Refractory / multi-pulse polish** (Can's latest ask): the 1.2 s lockout blocks rapid
   repeated pulses. To allow multi-pulse gestures, build a pulse-sequence detector. Trade-off
   vs open/close spam. Same machinery as item 2.
2. **Stage 2: 4 grip patterns** (strong/weak pulse PAIRS -> SS/WW/SW/WS -> 4 grip apertures)
   via a pulse-pairing state machine. The "wow" edge-AI version. Currently Stage 1.
3. **Port the DSP+DTW onto the STM32** (now on laptop) for a standalone edge-AI demo , the
   strongest portfolio line ("classifier on a bare-metal MCU").
4. **Gear math doc** (Can asked): `docs/gripper_mechanics.md` , gear ratio, output
   speed/torque, finger pinch force, servo-angle->aperture kinematics. NEEDS his final gear
   train (tooth counts + module he actually printed) + SG90 specs (~1.8 kg-cm stall, 0.1 s/60deg).
   ASK him for the final teeth/module first.
5. Demo video + portfolio writeup; CV update around the project (Can flagged the CV update).

## Git / build

All committed; CI green on push. Latest commit at handoff: `0bd4fdd`. Phase status in
`docs/phased_plan.md`. Threshold-control + scope history are in earlier commits.
