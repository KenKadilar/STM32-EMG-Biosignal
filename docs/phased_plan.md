# Phased plan (living checklist)

Source of truth for scope: `..\..\ChatAssistants\HealthAssistant\CareerAssistant\STM32_Project_Scope.md`.
This file tracks status. Check items off as they land. Hardware-gated phases stay
unchecked until the Nucleo-F446RE arrives.

Legend: [ ] not started, [~] in progress, [x] done, (HW) needs the board.

## Stage : Scaffold (no hardware needed)
- [x] PlatformIO project (`platformio.ini`, env `nucleo_f446re`, framework `stm32cube`)
- [x] Bare HAL `src/main.c` that compiles (toolchain proof)
- [x] `.gitignore`, README, docs, wiring plan
- [x] GitHub Actions CI (`pio run`)
- [ ] Private GitHub repo created + first push green in CI
- [ ] Toolchain pre-downloaded locally (`pio run` once, so Phase 0 is instant)

## 0. Toolchain (HW)
- [x] Plug in Nucleo over Mini-USB, ST-LINK enumerates (needed STSW-LINK009 driver for Code 28)
- [x] `pio run -t upload` flashes (no ST-LINK firmware upgrade needed; OpenOCD verified OK)
- [x] Blink LD2 (PA5) via HAL_GPIO : confirmed ~1 Hz on board 2026-06-05
- [ ] Step through with the onboard ST-LINK debugger (breakpoint, inspect) : optional, later

## 1. FreeRTOS
- [ ] Add FreeRTOS (kernel via middleware or lib_deps)
- [ ] 2 to 3 tasks: sampler / processor / comms
- [ ] Queues between tasks; confirm scheduling with a blinking task

## 2. EMG acquisition (HW)
- [ ] Timer-triggered ADC + DMA at a fixed sample rate
- [ ] Read Grove EMG analog envelope (PA0 / A0)
- [ ] Serial-plot the raw signal to confirm acquisition

## 3. DSP
- [ ] Enable CMSIS-DSP + hard FPU build flags
- [ ] Band-pass / rectify / envelope on-device
- [ ] Feature extraction (RMS / MAV / zero-crossings / waveform length)

## 4. Classification
- [ ] Port thesis classifier (DTW or lighter feature/threshold model)
- [ ] Output a discrete gesture class

## 5. Actuation (HW)
- [ ] PWM the SG90 servo (TIM channel) from the class
- [ ] Separate servo supply, common ground (see wiring)
- [ ] Closed-loop if a DC motor + encoder is reused

## 6. CAN (HW)
- [ ] MCP2515 over SPI (controller + transceiver module)
- [ ] Broadcast class / telemetry frames
- [ ] Decode frames on the 24 MHz logic analyzer (PulseView CAN decoder)
- [ ] Optional: SN65HVD230 on native bxCAN as a 2nd node for real send/receive

## 7. Safety
- [ ] IWDG watchdog
- [ ] Failsafe: motor off on signal loss / out-of-range
- [ ] Structured error handling (IEC 62304 mindset, no cert)

## 8. Ship
- [ ] README: scope + results + wiring
- [ ] CI green on every push
- [ ] Short demo video
- [ ] Add to portfolio (second hardware project); add STM32/FreeRTOS/CAN to CV embedded line
