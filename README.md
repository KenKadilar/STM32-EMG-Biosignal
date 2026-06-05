# STM32-EMG-Biosignal

On-device EMG gesture recognition on an STM32 Cortex-M4F. The board samples a surface
EMG sensor, filters and extracts features and classifies the gesture in firmware, drives
a motor from the result, and reports it over CAN, with a watchdog and failsafe. It is a
rebuild of an MSc prosthetic-hand thesis on real embedded firmware (STM32 + FreeRTOS +
CAN) instead of a PC or Arduino prototype.

> Status: SCAFFOLD. The board (Nucleo-F446RE) is not in hand yet. The repo builds a bare
> HAL firmware so the toolchain and CI are proven before hardware arrives. Real firmware
> lands phase by phase once the parts come in. See `docs/phased_plan.md`.

## Hardware

| Part | Role |
|---|---|
| ST Nucleo-F446RE | STM32F446RET6, Cortex-M4F @180 MHz, FPU + DSP, 2x bxCAN, onboard ST-LINK |
| Grove EMG detector | analog EMG envelope into the ADC |
| MCP2515 module | CAN controller + transceiver, driven over SPI |
| 24 MHz USB logic analyzer | decode SPI / UART / CAN (oscilloscope substitute for digital) |
| TowerPro SG90 servo | actuation from the classified gesture |

Wiring plan: `docs/wiring.md` (draft, to confirm on the board).

## Toolchain

PlatformIO with `framework = stm32cube` (the real ST HAL + CMSIS, not Arduino). Builds
from VS Code or the CLI, flashes and debugs over the onboard ST-LINK.

```
pio run              # build firmware
pio run -t upload    # flash over ST-LINK (needs the board)
pio device monitor   # serial @115200
```

CI builds the firmware on every push (`.github/workflows/build.yml`).

## Layout

```
platformio.ini            PlatformIO config (board nucleo_f446re, stm32cube)
src/main.c                firmware entry (scaffold stub for now)
lib/                      portable algorithm modules (EMG DSP, classifier) land here
include/                  shared headers
test/                     host/unit tests
docs/phased_plan.md       living checklist (phases 0 to 8)
docs/wiring.md            pin map + power plan
.github/workflows/        CI (pio run)
```

## Plan

Phases 0 to 8: toolchain, FreeRTOS, EMG acquisition (ADC + DMA), CMSIS-DSP, classification,
actuation, CAN, safety (IWDG + failsafe), ship. Full breakdown and status in
`docs/phased_plan.md`.
