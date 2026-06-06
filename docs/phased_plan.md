# Phased plan (living checklist)

Source of truth for scope: `..\..\ChatAssistants\HealthAssistant\CareerAssistant\STM32_Project_Scope.md`.
This file tracks status. Check items off as they land.

Legend: [ ] not started, [~] in progress, [x] done, (HW) needs the board.

Note: the build order was reordered from the scope's numbering to "working core first"
(EMG acquisition + DSP + servo before FreeRTOS/CAN), per Can's preference. Phase numbers
below keep the scope's labels; the checkmarks reflect actual order done.

## Stage : Scaffold
- [x] PlatformIO project (`platformio.ini`, env `nucleo_f446re`, framework `stm32cube`)
- [x] Bare HAL `src/main.c` that compiles (toolchain proof)
- [x] `.gitignore`, README, docs, wiring plan
- [x] GitHub Actions CI (`pio run`)
- [x] Private GitHub repo created + first push green in CI
- [x] Toolchain pre-downloaded locally

## 0. Toolchain (HW)
- [x] Plug in Nucleo over Mini-USB, ST-LINK enumerates (needed STSW-LINK009 driver for Code 28)
- [x] `pio run -t upload` flashes (no ST-LINK firmware upgrade needed)
- [x] Blink LD2 (PA5) : confirmed ~1 Hz 2026-06-05
- [ ] Step through with the ST-LINK debugger : optional, later

## 2. EMG acquisition (HW)  DONE
- [x] Timer-triggered ADC at 1 kHz (TIM2 ISR). DMA deferred (polling read is fine at 1 kHz).
- [x] Read Grove EMG analog envelope (PA0 / A0)
- [x] Serial-plot / log the raw signal; characterized noise (see docs/emg_noise_findings.md)

## 3. DSP  DONE (custom, no CMSIS-DSP needed)
- [x] Adaptive bias removal (live DC tracking, not hardcoded)
- [x] 50 Hz notch biquad (mains rejection on the linear signal)
- [x] Rectify + moving-average envelope on-device
- [ ] CMSIS-DSP : NOT needed (Grove filters internally; custom pipeline suffices)
- [ ] Feature extraction (RMS/MAV/ZC/WL) : only if DTW needs it

## 4. Classification
- [ ] Port thesis DTW (2 templates: open / close) + calibration
- [ ] Threshold + hysteresis as the first loop-closer (simpler than DTW; smoke test)
- [ ] Output a discrete gesture class

## 5. Actuation (HW)  servo working
- [x] PWM the SG90 servo on TIM4_CH1 / PB6 (D10); 90 deg sweep confirmed 2026-06-06
- [x] Separate 6 V servo supply, shared ground (no brownout)
- [ ] Drive position from the gesture class (integration step, next)
- [ ] Closed-loop with encoder : not planned (SG90 chosen; see scope)

## 6. CAN (HW)
- [ ] MCP2515 over SPI (controller + transceiver module)
- [ ] Broadcast class / telemetry frames
- [ ] Decode frames on the 24 MHz logic analyzer (PulseView CAN decoder)
- [ ] Optional: SN65HVD230 on native bxCAN as a 2nd node

## 7. Safety  partly done early
- [x] Signal-fault detection (clip / over-amplitude / bias out of range) + LD2 warning +
      auto raw-capture; validated against induced faults (see emg_noise_findings.md)
- [ ] IWDG watchdog
- [ ] Wire the fault state into a motor-off failsafe (once EMG drives the servo)

## 1. FreeRTOS  (after the working core)
- [ ] Add FreeRTOS (kernel via middleware or lib_deps)
- [ ] Tasks: sampler / processor / comms + queues
- [ ] Refactor the working super-loop into tasks

## 8. Ship
- [ ] README: scope + results + wiring
- [x] CI green on every push
- [ ] Short demo video
- [ ] Add to portfolio; add STM32/CAN/RTOS to the CV embedded line
