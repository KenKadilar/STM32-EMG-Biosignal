# Phased plan (build checklist) , COMPLETE

All phases done and verified on hardware (2026-06-14). Kept as a record of the build order.

Legend: `[x]` done · `[ ]` deliberately not done (noted).

## 0. Toolchain
- [x] PlatformIO + `stm32cube`, ST-LINK flash/debug over Mini-USB
- [x] Blink LD2 (toolchain proof); GitHub Actions CI (`pio run`) green on every push

## Sensing , EMG acquisition
- [x] TIM2-triggered ADC + DMA at 1 kHz (`Emg.h`), no CPU per sample
- [x] Read Grove EMG on PA0; characterized noise (`docs/emg_noise_findings.md`)

## DSP
- [x] 50 Hz mains-rejection notch biquad (`Notch.h`) on the centered signal at 1 kHz
- [ ] CMSIS-DSP library swap , optional, the notch already demonstrates on-chip DSP

## Classification (on-chip)
- [x] Baseline tracking + centering + dip detector (`MuscleTrigger.h`): one flex = one toggle
- [x] Signal-loss failsafe (railing/unplugged -> hold, re-trust after ~1 s)
- DTW multi-grip stays in the thesis by design; this project's classifier is the lightweight dip detector

## Actuation
- [x] PWM the SG90 servo (TIM4_CH1 / PB6), slew-limited ease; separate 6 V supply, shared ground
- [x] Driven by the on-chip gesture (integrated)

## FreeRTOS
- [x] Kernel integrated (ARM_CM4F port, hard-float wiring: `platformio.ini` + `fpu_link.py`)
- [x] 4 tasks (servo / comms / can / watchdog) + 2 queues (`mailBox`, `canMailBox`); brain in the DMA ISR
- [x] Watchdog as the lowest-priority task = watchdog-starvation reboot

## CAN
- [x] Hand-written MCP2515-over-SPI driver (`Mcp2515CanBus.h` + `Mcp2515Registers.h`)
- [x] Two-node transmit verified against the Arduino node (`bench/arduino_can_node/`)
- [x] Integrated: gesture frame (0x100) + status heartbeat (0x101)
- [ ] RX + hardware ID filter , deliberately skipped (off-narrative for a TX-only node; see README limitations)

## Safety
- [x] Signal-loss failsafe (`MuscleTrigger.h`)
- [x] IWDG watchdog (~2 s), verified by a deliberate hang test

## Ship
- [x] README (scope + architecture + the CAN story + mechanical + BOM + contact)
- [x] Demo video (`docs/demo_video_STM32.mp4`)
- [x] CAD render + animation GIF + printable STLs (`cad/`, `docs/`)
- [x] CI green on every push
- [ ] Flip repo public + feature on canarchive.com + add STM32 / FreeRTOS / CAN to the CV embedded line
