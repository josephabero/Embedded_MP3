#pragma once

#include <cstdint>

#include <stdint.h>
#include <stdbool.h>

#include "source/gpio.hpp"

#include "utility/log.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

namespace SpiLab
{
// Frameformat
class AT25SF041 {
  public:
    enum class Commands : uint8_t
    {
      kManufacturerAndDeviceId = 0,
      kReadBuffer = 1,
    };

    // Constructor(pins)
    AT25SF041(SpiLab::Gpio* CS){
      spi = SpiLab::Spi();
      chip_select = CS;
    }

    void Initialize(){
      const uint8_t frame_data_size = 8;
      const uint8_t clock_divider = 48;

      chip_select->SetAsOutput();
      chip_select->SetHigh();       // Initialize CS to Disabled

      spi.Initialize(SpiLab::Spi::SpiPort::kPort2,
                     frame_data_size,
                     SpiLab::Spi::FrameFormat::kSPI,
                     clock_divider,
                     SpiLab::Spi::MasterSlaveMode::kMaster);
    }

    std::vector<uint32_t> TransferBuffer(std::vector<uint32_t> buffer){
      std::vector<uint32_t> result;
      for (uint32_t data : buffer) {
        sjsu::LogInfo("Sending: 0x%02x...", data);
        result.push_back(spi.Transfer(data));
      }
      return result;
    }

    std::vector<uint32_t> SendCommand(Commands command){
      std::vector<uint32_t> buffer;

      switch(command){
        case Commands::kManufacturerAndDeviceId:
          buffer.push_back(0x9F);
          buffer.push_back(0x00);
          buffer.push_back(0x00);
          break;
        case Commands::kReadBuffer:
          buffer.push_back(0x10);
          break;
        default:
          sjsu::LogError("Invalid AT25SF041 Command: 0x%02x...", command);
          exit(1);
      }


      chip_select->SetLow();
      // Transfer Buffer
      std::vector<uint32_t> result = TransferBuffer(buffer);
      chip_select->SetHigh();

      return result;
    }

  private:
    SpiLab::Spi spi;
    SpiLab::Gpio* chip_select;
};
}   // namespace SpiLab