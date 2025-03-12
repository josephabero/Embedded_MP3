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

    // void SendCommand(Commands command, uint8_t* buffer){
    //   chip_select->SetLow();
    //   spi.Transfer(buffer);
    //   chip_select->SetHigh();
    // }

  private:
    SpiLab::Spi spi;
    SpiLab::Gpio* chip_select;
};
}   // namespace SpiLab