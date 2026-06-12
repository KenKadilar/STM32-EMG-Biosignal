// Mcp2515CanBus : the CAN-bus controller (an MCP2515 chip on the end of SPI2). The STM32 controls it
// entirely by reading/writing its registers over SPI. Step 1 only brings up SPI + reads one register.
#ifndef MCP2515_CAN_BUS_H
#define MCP2515_CAN_BUS_H
#include "stm32f4xx_hal.h"

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
        uint8_t toSend[3] = { CMD_WRITE, address, value };
        select();
        HAL_SPI_Transmit(&hspi, toSend, 3, 100);
        deselect();
    }

    // Read one register: clock out [READ, address, dummy]; the chip clocks the value back during the 3rd byte.
    uint8_t readRegister(uint8_t address)
    {
        uint8_t toSend[3]    = { CMD_READ, address, 0x00 };   // 3rd byte is dummy: it just clocks the answer out
        uint8_t received[3]  = { 0, 0, 0 };
        select();
        HAL_SPI_TransmitReceive(&hspi, toSend, received, 3, 100);
        deselect();
        return received[2];                          // the register value rode back in on the 3rd byte
    }

    uint8_t readCanstat() { return readRegister(REG_CANSTAT); }   // 0x80 right after reset = "Configuration mode"

    // Set the wire speed (8 MHz crystal @ 500 kbps). Every node on a CAN bus MUST agree on this exactly,
    // or they can't decode each other. Writable only while in Configuration mode (i.e. right after reset).
    void setBitTiming8MHz500k()
    {
        writeRegister(REG_CNF1, CNF1_8MHZ_500K);
        writeRegister(REG_CNF2, CNF2_8MHZ_500K);
        writeRegister(REG_CNF3, CNF3_8MHZ_500K);
    }

    // Leave Configuration mode and go live: REQOP = 000 in CANCTRL = Normal mode. After this the chip
    // participates on the bus, and CANSTAT's mode bits read 000 (so CANSTAT = 0x00).
    void enterNormalMode()
    {
        writeRegister(REG_CANCTRL, 0x00);   // 0x00: Normal mode, one-shot off, CLKOUT off
    }

  private:
    void select()   { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); }   // CS LOW  = start talking
    void deselect() { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   }   // CS HIGH = done talking

    // MCP2515 SPI command bytes + the one register we read in step 1 (from the chip's datasheet).
    static const uint8_t CMD_RESET  = 0xC0;   // reset the whole chip to a known state
    static const uint8_t CMD_READ   = 0x03;   // "read register at the address that follows"
    static const uint8_t CMD_WRITE  = 0x02;   // "write the value that follows to the address that follows"
    static const uint8_t REG_CANSTAT = 0x0E;  // status register; top 3 bits = current operating mode
    static const uint8_t REG_CANCTRL = 0x0F;  // control register; top 3 bits = REQUESTED operating mode
    static const uint8_t REG_CNF3    = 0x28;  // bit-timing config 3
    static const uint8_t REG_CNF2    = 0x29;  // bit-timing config 2
    static const uint8_t REG_CNF1    = 0x2A;  // bit-timing config 1

    // Bit timing for an 8 MHz crystal @ 500 kbps. These exact bytes are the ones the Arduino bench used
    // (coryjfowler MCP_CAN), so the STM32 node and the Arduino node agree on the wire speed.
    static const uint8_t CNF1_8MHZ_500K = 0x00;
    static const uint8_t CNF2_8MHZ_500K = 0x90;
    static const uint8_t CNF3_8MHZ_500K = 0x82;

    SPI_HandleTypeDef hspi = {};
};
#endif
