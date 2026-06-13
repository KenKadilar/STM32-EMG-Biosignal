// Mcp2515CanBus : the CAN-bus controller (an MCP2515 chip on the end of SPI2). The STM32 controls it
// entirely by reading/writing its registers over SPI. All the command/register/value bytes live in
// Mcp2515Registers.h (the datasheet, as code); this file is the logic that uses them.
#ifndef MCP2515_CAN_BUS_H
#define MCP2515_CAN_BUS_H
#include "stm32f4xx_hal.h"
#include "Mcp2515Registers.h"

class Mcp2515CanBus
{
  public:
    void init()
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();      // SCK/MISO/MOSI/CS all live on port B
        __HAL_RCC_SPI2_CLK_ENABLE();       // turn on power to SPI peripheral 2

        // PB12 = chip-select. A plain output WE drive by hand (not the SPI block). Idle HIGH = "not talking".
        GPIO_InitTypeDef cs = {0};
        cs.Pin = GPIO_PIN_12; cs.Mode = GPIO_MODE_OUTPUT_PP; cs.Pull = GPIO_NOPULL;
        cs.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(GPIOB, &cs);
        deselect();                        // start HIGH so the chip isn't selected until we want it

        // PB13/14/15 = SCK/MISO/MOSI, handed to the SPI2 peripheral ("alternate function 5").
        GPIO_InitTypeDef spi = {0};
        spi.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        spi.Mode = GPIO_MODE_AF_PP; spi.Pull = GPIO_NOPULL;
        spi.Speed = GPIO_SPEED_FREQ_VERY_HIGH; spi.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &spi);

        hspi.Instance               = SPI2;
        hspi.Init.Mode              = SPI_MODE_MASTER;          // the STM32 drives the clock
        hspi.Init.Direction         = SPI_DIRECTION_2LINES;    // separate MOSI + MISO (full duplex)
        hspi.Init.DataSize          = SPI_DATASIZE_8BIT;       // one byte at a time
        hspi.Init.CLKPolarity       = SPI_POLARITY_LOW;        // SPI mode 0: clock rests low...
        hspi.Init.CLKPhase          = SPI_PHASE_1EDGE;         // ...and data is read on the first (rising) edge
        hspi.Init.NSS               = SPI_NSS_SOFT;            // we do chip-select ourselves (PB12), not the block
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 16 MHz / 8 = 2 MHz: gentle + easy for the analyzer
        hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;        // most-significant bit first (what the MCP2515 expects)
        hspi.Init.TIMode            = SPI_TIMODE_DISABLE;
        hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
        HAL_SPI_Init(&hspi);
    }

    // Send the one-byte RESET command, then wait for the chip to finish its internal reset. After this
    // the MCP2515 is in Configuration mode, so CANSTAT reads 0x80.
    void reset()
    {
        uint8_t command = CMD_RESET;
        select();
        HAL_SPI_Transmit(&hspi, &command, 1, 100);   // 100 = give-up timeout (ms)
        deselect();
        HAL_Delay(10);                               // let the reset settle before any more SPI
    }

    // Write one register: clock out [WRITE, address, value] in one CS-low...CS-high. The chip has no
    // reply for a write, so this is a plain transmit (no dummy byte, nothing to read back here).
    void writeRegister(uint8_t address, uint8_t value)
    {
        uint8_t toSend[3] = { CMD_WRITE_REGISTER, address, value };
        select();
        HAL_SPI_Transmit(&hspi, toSend, 3, 100);
        deselect();
    }

    // Read one register: clock out [READ, address, dummy]; the chip clocks the value back during the 3rd byte.
    uint8_t readRegister(uint8_t address)
    {
        uint8_t toSend[3]    = { CMD_READ_REGISTER, address, 0x00 };   // 3rd byte is dummy: it just clocks the answer out
        uint8_t received[3]  = { 0, 0, 0 };
        select();
        HAL_SPI_TransmitReceive(&hspi, toSend, received, 3, 100);
        deselect();
        return received[2];                          // the register value rode back in on the 3rd byte
    }

    uint8_t readCanstat() { return readRegister(STATUS_REGISTER); }   // 0x80 right after reset = "Configuration mode"

    // Set the wire speed (8 MHz crystal @ 500 kbps). Every node on a CAN bus MUST agree on this exactly,
    // or they can't decode each other. Writable only while in Configuration mode (i.e. right after reset).
    void setBitTiming8MHz500k()
    {
        writeRegister(BIT_TIMING_1, BIT_TIMING_1_VALUE_8MHZ_500K);
        writeRegister(BIT_TIMING_2, BIT_TIMING_2_VALUE_8MHZ_500K);
        writeRegister(BIT_TIMING_3, BIT_TIMING_3_VALUE_8MHZ_500K);
    }

    // Leave Configuration mode and go live: REQOP = 000 in CANCTRL = Normal mode. After this the chip
    // participates on the bus, and CANSTAT's mode bits read 000 (so CANSTAT = 0x00).
    void enterNormalMode() { writeRegister(CONTROL_REGISTER, MODE_NORMAL); }

    // Loopback mode: the chip routes its own transmit straight into its own receiver, internally.
    // No bus wires, no second node, no ACK needed. Perfect for testing frame code alone.
    void enterLoopbackMode() { writeRegister(CONTROL_REGISTER, MODE_LOOPBACK); }

    // Tell receive buffer 0 to accept ANY frame (turns the ID filters/masks off). Without it, the
    // freshly-reset filters could drop our frame.
    void acceptAllOnRxBuffer0() { writeRegister(RX_BUFFER0_CONTROL, RX_ACCEPT_ANY); }

    // Build a standard (11-bit ID) data frame in TX buffer 0 and fire it. length = 0..8 data bytes.
    void sendFrame(uint16_t id, const uint8_t *data, uint8_t length)
    {
        writeRegister(TX_BUFFER0_ID_HIGH, (uint8_t)(id >> 3));            // the ID's top 8 bits
        writeRegister(TX_BUFFER0_ID_LOW,  (uint8_t)((id & 0x07) << 5));   // the ID's bottom 3 bits, parked in bits 7..5
        writeRegister(TX_BUFFER0_LENGTH, length);                        // how many data bytes follow
        for (uint8_t i = 0; i < length; i++)
            writeRegister(TX_BUFFER0_DATA0 + i, data[i]);                // the payload, byte by byte
        requestToSendTxb0();                                            // "send it now"
    }

    // Did a frame land in receive buffer 0? RX0IF is bit 0 of the interrupt-flag register.
    bool messageWaiting() { return (readRegister(INTERRUPT_FLAGS) & 0x01) != 0; }

    // Read the frame out of receive buffer 0 into id + data[]; returns the data length. Clears the flags
    // afterwards so the next frame can be detected.
    uint8_t readFrame(uint16_t *id, uint8_t *data)
    {
        uint8_t sidh = readRegister(RX_BUFFER0_ID_HIGH);
        uint8_t sidl = readRegister(RX_BUFFER0_ID_LOW);
        *id = (uint16_t)(sidh << 3) | (sidl >> 5);     // rebuild the 11-bit ID from its two halves
        uint8_t length = readRegister(RX_BUFFER0_LENGTH) & 0x0F;
        for (uint8_t i = 0; i < length; i++)
            data[i] = readRegister(RX_BUFFER0_DATA0 + i);
        writeRegister(INTERRUPT_FLAGS, CLEAR_ALL_FLAGS);   // clear flags (incl. RX0IF) so the next frame shows
        return length;
    }

  private:
    void select()   { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); }   // CS LOW  = start talking
    void deselect() { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   }   // CS HIGH = done talking

    // The "request to send" instruction is a single command byte (no address), unlike read/write.
    void requestToSendTxb0()
    {
        uint8_t command = CMD_REQUEST_TO_SEND_TX0;
        select();
        HAL_SPI_Transmit(&hspi, &command, 1, 100);
        deselect();
    }

    SPI_HandleTypeDef hspi = {};
};
#endif
