#pragma once

#include <cstdint>

#include <stdint.h>
#include <stdbool.h>

#include "utility/log.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

namespace SpiLab
{
// Frameformat
class Adesto {
  public:
    // Constructor(pins)
    VS1053::VS1053(LabGPIO* xdcs, LabGPIO* xcs, LabGPIO* rst, LabGPIO* dreq)
    {
      XDCS = xdcs;
      XCS = xcs;
      RST = rst;
      DREQ = dreq;
    }
    // Enable
    // Transfer
  private:

}   // namespace SpiLab