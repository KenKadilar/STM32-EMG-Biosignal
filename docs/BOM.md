# Bill of materials

Parts to reproduce this build. Most were bought from **Robotistan** (Istanbul) on **2026-06-04**;
prices are in Turkish Lira as paid, treat as indicative (they drift, and the build is two CAN nodes,
so it includes a second MCP2515 and an Arduino).

| Part | Qty | Role | Price (TL, as paid) |
|---|---:|---|---:|
| ST Nucleo-F446RE | 1 | the MCU board (Cortex-M4F, onboard ST-LINK) | 1380.12 |
| MCP2515 CAN module (controller + transceiver, SPI) | 2 | one per CAN node (STM32 side + Arduino bench node) | 205.97 |
| TowerPro SG90 servo, 180° | 2 | gripper actuation (clones arrive weak, the 2nd is a spare) | 156.72 |
| USB logic analyzer, 24 MHz 8-channel | 1 | SPI/CAN protocol decode (oscilloscope substitute for digital) | 364.92 |
| Mini-USB cable, 150 cm | 1 | powers + flashes the Nucleo (Mini-USB, not micro/USB-C) | 43.66 |
| Jumper wires F-F, 200 mm (40-pin) | 1 | breadboard wiring | 45.34 |
| Jumper wires M-M, 200 mm (40-pin) | 1 | breadboard wiring | 55.41 |
| Jumper wires M-F, 200 mm (40-pin) | 1 | breadboard wiring | 52.61 |
| Grove EMG Detector (Seeed) | 1 | analog EMG envelope into the ADC (already owned) | ~$30 USD |

**Already on hand (not re-bought):** a breadboard, EMG electrodes, a 6 V battery pack (4×AAA) for the
servo, an **Arduino UNO** for the second CAN node, a multimeter, and ~120 Ω of bus termination.

> Grove EMG Detector: owned from the thesis work; Seeed list price is ~$30 USD (about €29). No
> oscilloscope is needed, the logic analyzer covers the digital buses and the analog EMG is read via the
> ADC and plotted from serial.
