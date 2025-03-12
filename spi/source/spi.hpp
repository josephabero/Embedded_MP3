#pragma once

#include <cstdint>

#include <stdint.h>
#include <stdbool.h>

#include "utility/log.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

namespace SpiLab
{
// Frameformat
class Spi {
  public:
    enum class SpiPort : uint8_t
    {
      kPort0 = 0,
      kPort1 = 1,
      kPort2 = 2,
    };

    enum class FrameFormat : uint8_t
    {
      kSPI       = 0,
      kTI        = 1,
      kMicrowire = 2
    };

    enum class MasterSlaveMode : uint8_t
    {
      kMaster = 0,
      kSlave  = 1,
    };

    enum class LoopBackMode : uint8_t
    {
      kNormalOperation = 0,
      kUseSerialOutput = 1,
    };

    typedef union
    {
      uint8_t byte;
      struct
      {
        uint8_t data_ready: 1;
        uint8_t fifo_ready: 1;
        uint8_t fifo_warning: 1;
        uint8_t fifo_overrun: 1;
        uint8_t activity: 1;
        uint8_t inactivity: 1;
        uint8_t awake: 1;
        uint8_t error: 1;
      } __attribute__((packed));
    } adlx_t;

    typedef union
    {
      uint32_t reg;
      struct
      {
        uint8_t data_size: 4;
        uint8_t frame_format: 2;
        uint8_t polarity: 1;
        uint8_t phase: 1;
        uint8_t clock_rate: 8;
        uint32_t : 16;
      } __attribute__((packed));
    } ControlRegister0;

    typedef union
    {
      uint32_t reg;
      struct
      {
        uint8_t loop_back_mode: 1;
        uint8_t enable: 1;
        uint8_t mode: 1;
        uint8_t output_disable: 1;
        uint32_t : 28;
      } __attribute__((packed));
    } ControlRegister1;

    // Constructor
    Spi() {}

    void Initialize(SpiPort port, uint8_t frame_data_size, FrameFormat frame_format, uint8_t freq_divider, MasterSlaveMode master_slave_mode){
      sjsu::LogInfo("Initializing SPI Driver...");
      if(freq_divider < 2 || freq_divider & (1 << 0)){
        sjsu::LogError("SPI Clock Divider is an Odd Number or is Out of Range (2 <= divide <= 32). Divider: %d", freq_divider);
        exit(1);
      }

      // Enable the Port
      switch(port){
        case SpiPort::kPort0:
          sjsu::LogInfo("Enable SSP0");
          sjsu::LogInfo("Initializing P0.15 as SSP0_SCLK");
          sjsu::LogInfo("Initializing P0.18 as SSP0_MOSI");
          sjsu::LogInfo("Initializing P0.17 as SSP0_MISO");

          LPC_SSPx = sjsu::lpc40xx::LPC_SSP0;

          sjsu::lpc40xx::LPC_SC->PCONP |= (1 << 21);                            // Power on SSP0 Interface
          sjsu::lpc40xx::LPC_IOCON->P0_15 = (sjsu::lpc40xx::LPC_IOCON->P0_15 & ~(0b111)) | (0b010); // SCLK0
          sjsu::lpc40xx::LPC_IOCON->P0_18 = (sjsu::lpc40xx::LPC_IOCON->P0_18 & ~(0b111)) | (0b010); // MOSI0
          sjsu::lpc40xx::LPC_IOCON->P0_17 = (sjsu::lpc40xx::LPC_IOCON->P0_17 & ~(0b111)) | (0b010); // MISO0
          break;
        case SpiPort::kPort1:
          sjsu::LogInfo("Enable SSP1");
          sjsu::LogInfo("Initializing P0.7 as SSP1_SCLK");
          sjsu::LogInfo("Initializing P0.9 as SSP1_MOSI");
          sjsu::LogInfo("Initializing P0.8 as SSP1_MISO");

          LPC_SSPx = sjsu::lpc40xx::LPC_SSP1;

          sjsu::lpc40xx::LPC_SC->PCONP |= (1 << 10);                          // Power on SSP1 Interface
          sjsu::lpc40xx::LPC_IOCON->P0_7 = (sjsu::lpc40xx::LPC_IOCON->P0_7 & ~(0b111)) | (0b010); // SCLK1
          sjsu::lpc40xx::LPC_IOCON->P0_9 = (sjsu::lpc40xx::LPC_IOCON->P0_9 & ~(0b111)) | (0b010); // MOSI1
          sjsu::lpc40xx::LPC_IOCON->P0_8 = (sjsu::lpc40xx::LPC_IOCON->P0_8 & ~(0b111)) | (0b010); // MISO1
          break;
        case SpiPort::kPort2:
          sjsu::LogInfo("Enable SSP2");
          sjsu::LogInfo("Initializing P1.0 as SSP2_SCLK");
          sjsu::LogInfo("Initializing P1.1 as SSP2_MOSI");
          sjsu::LogInfo("Initializing P1.4 as SSP2_MISO");

          sjsu::lpc40xx::LPC_SC->PCONP |= (1 << 20);                          // Power on SSP2 Interface
          sjsu::lpc40xx::LPC_IOCON->P1_0 = (sjsu::lpc40xx::LPC_IOCON->P1_0 & ~(0b111)) | 0b100; // SCLK2
          sjsu::lpc40xx::LPC_IOCON->P1_1 = (sjsu::lpc40xx::LPC_IOCON->P1_1 & ~(0b111)) | 0b100; // MOSI2
          sjsu::lpc40xx::LPC_IOCON->P1_4 = (sjsu::lpc40xx::LPC_IOCON->P1_4 & ~(0b111)) | 0b100; // MISO2

          LPC_SSPx = sjsu::lpc40xx::LPC_SSP2;
          break;
        default:
          sjsu::LogError("Invalid SPI port: %d", port);
          exit(1);
          break;
      }

      // // Set the SPI Clock Frequency
      // LPC_SSPx->CPSR |= freq_divider;

      // // Setup Control Registers
      // ControlRegister0 CR0;
      // CR0.frame_format = static_cast<int>(frame_format);
      // CR0.data_size = static_cast<int>(frame_data_size);

      // ControlRegister1 CR1;
      // CR1.loop_back_mode = static_cast<int>(LoopBackMode::kNormalOperation);
      // CR1.mode = static_cast<char>(master_slave_mode);

      // // Set Control Registers
      // LPC_SSPx->CR0 = CR0.reg;
      // LPC_SSPx->CR1 = CR1.reg;
      // Initially clear CR0
      LPC_SSPx->CR0 = 0;
      LPC_SSPx->CR0 |= (0b111);       // Set DSS to user defined bit mode
      LPC_SSPx->CR0 |= 0;                         // Sets format based on user definition
      LPC_SSPx->CR0 &= ~(1 << 6);                 // Clear CPOL, bus clock low between frames
      LPC_SSPx->CR0 &= ~(1 << 7);                 // Clear CPHA, first transition
      LPC_SSPx->CR0 &= ~(0xFFFF << 8);            // Clear SCR to 0
      LPC_SSPx->CPSR |= freq_divider;             // Set CPSR to user divide, ONLY ALLOWS FOR EVEN NUMBERS TO DIVIDE

      // Set CR1
      LPC_SSPx->CR1 &= ~(1);           // Set to normal operation
      LPC_SSPx->CR1 &= ~(1 << 2);      // Set SSP2 as Master
      LPC_SSPx->CR1 |= (1 << 1);       // Enable SSP2
    }

    uint32_t Transfer(uint32_t send)
    {
        uint32_t result_byte = 0;

        // Set SSP2 Data Register to send value
        LPC_SSPx->DR = send;

        while(LPC_SSPx->SR & (1 << 4))
        {
            continue;   // BSY is set, currently sending/receiving frame
        }

        // When BSY bit is set, SSP2 Data Register holds value read from d
        result_byte = LPC_SSPx->DR;
        return result_byte;
    }
  private:
    sjsu::lpc40xx::LPC_SSP_TypeDef* LPC_SSPx;
};
}   // namespace SpiLab