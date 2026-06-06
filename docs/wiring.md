# Wiring plan : Nucleo-F446RE (DRAFT)

Status: DRAFT. Pins below are a sensible default to start from, NOT yet verified on
the board. Confirm against the Nucleo-F446RE pinout (UM1724) and CubeMX when you wire
it up in Phase 0+. The point of writing it now is to think the I/O through before the
parts land, not to lock it.

Board pinout reference: `STM32_NUCLEO_F446RE_Pinout.png` (in this folder). Note the
two header styles: Arduino headers (CN5/6/8/9, nicknames like A0/D13) and Morpho
headers (CN7/CN10, real chip names like PA0). A0 and PA0 are the SAME physical pin,
two labels. The firmware always uses the chip name (PA0 = GPIOA pin 0).

## Pin map (proposed)

| Function | Signal | STM32 pin | Nucleo header | Notes |
|---|---|---|---|---|
| EMG sensor 1 | ADC1_IN0 | PA0 | A0 | Grove EMG analog envelope in |
| EMG sensor 2 | ADC1_IN1 | PA1 | A1 | second channel (optional) |
| Servo PWM | TIM4_CH1 | PB6 | D10 | SG90 signal (3.3 V PWM is fine) |
| Debug UART TX | USART2_TX | PA2 | (ST-LINK VCP) | prints to PC over the Nucleo USB |
| Debug UART RX | USART2_RX | PA3 | (ST-LINK VCP) | |
| Onboard LED | GPIO | PA5 | D13 / LD2 | blink target for Phase 0 |
| MCP2515 SCK | SPI2_SCK | PB13 | CN10 | SPI2 chosen to avoid the PA5/LD2 clash |
| MCP2515 MISO | SPI2_MISO | PB14 | CN10 | |
| MCP2515 MOSI | SPI2_MOSI | PB15 | CN10 | |
| MCP2515 CS | GPIO out | PB12 | CN10 | software chip-select |
| MCP2515 INT | GPIO in (EXTI) | PA9 | D8 | RX interrupt from controller |
| (opt) native CAN RX | CAN1_RX | PB8 | D15 | only if adding SN65HVD230 |
| (opt) native CAN TX | CAN1_TX | PB9 | D14 | only if adding SN65HVD230 |

Why SPI2 and not SPI1: SPI1's SCK is PA5, which is also LD2 (the onboard LED). It works,
but the LED flickers with every SPI clock. SPI2 (PB13/14/15) keeps them separate.

## Power

- Nucleo: powered + flashed over Mini-USB from the PC (ST-LINK). Mini-USB, not micro,
  not USB-C.
- MCP2515 module: 5 V from the Nucleo 5V pin + GND (low current, fine on USB power).
  Logic is 3.3 V tolerant on most modules; verify your module before trusting MOSI/CS.
- SG90 servo: SEPARATE supply, do NOT run it off the Nucleo 5V pin (current spikes
  brown out the board). 4xAAA (6 V) pack straight to the servo is simplest. CRITICAL:
  tie the servo supply ground to Nucleo GND (battery minus to a GND pin) so the PWM
  signal has a common reference. Without the shared ground the servo will twitch or
  ignore the signal.

## CAN bus

MCP2515-over-SPI is the primary CAN path (the F446 talks SPI to the MCP2515, which is
controller + transceiver). To show traffic with one node: decode the CAN frames on the
24 MHz logic analyzer in PulseView (CAN decoder). For a real two-node exchange: add a
second MCP2515 on an Arduino, or a $3 SN65HVD230 on the F446's native bxCAN (PB8/PB9).

Note: F446 native CAN is bxCAN (classic CAN 2.0), NOT FDCAN. FDCAN is G4/H7 only.
