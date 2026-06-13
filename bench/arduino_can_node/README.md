# Arduino CAN bench node

The **second CAN node** used to verify the STM32's CAN link end to end: first to prove the two MCP2515
modules + bus worked at all (before any STM32 CAN code existed), then as the receiver in the live
two-node demo. An Arduino UNO + an MCP2515 module (8 MHz crystal, 500 kbps), matching the STM32 side.

Note the deliberate contrast: this side uses the off-the-shelf **coryjfowler MCP_CAN** library, while the
STM32 side runs a **hand-written, register-level MCP2515 driver** ([`src/Mcp2515CanBus.h`](../../src/Mcp2515CanBus.h)
and [`src/Mcp2515Registers.h`](../../src/Mcp2515Registers.h)). The Arduino is only a known-good test partner.

- `src/receiver.cpp` (env `receiver`): prints every received frame; decodes the gripper's status (`0x101`)
  and gesture (`0x100`) frames into plain text (`servo=OPEN/CLOSED  electrode=ATTACHED/DETACHED`).
- `src/sender.cpp` (env `sender`): transmits test frames, used to validate the modules on the bench.

Build/flash one role per UNO (PlatformIO):

```
pio run -e receiver -t upload
pio run -e sender   -t upload
pio device monitor -e receiver
```

Wiring: MCP2515 over SPI (UNO D13 SCK / D11 SI / D12 SO / D10 CS / D2 INT, 5V + GND); CANH-CANH and
CANL-CANL to the other node, shared ground. See [`../../docs/wiring.md`](../../docs/wiring.md).
