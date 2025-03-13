#pragma once 
#ifndef LABSPI_H_
#define LABSPI_H_

#include "L0_LowLevel/LPC40xx.h"
#include "LabGPIO.hpp"
#include <cstdint>

class LabSpi
{
 public:
    enum FrameModes : uint8_t
    {
        kSPI        = 0b00,
        kTI         = 0b01,
        kMicrowire  = 0b10
    };

    enum SPI_Port : uint8_t
    {
        kPort0      = 0,
        kPort1      = 1,
        kPort2      = 2,
    };
    // *
    //  * 1) Powers on SPPn peripheral
    //  * 2) Set peripheral clock
    //  * 3) Sets pins for specified peripheral to MOSI, MISO, and SCK
    //  *
    //  * @param data_size_select transfer size data width; To optimize the code, look for a pattern in the datasheet
    //  * @param format is the code format for which synchronous serial protocol you want to use.
    //  * @param divide is the how much to divide the clock for SSP; take care of error cases such as the value of 0, 1, and odd numbers
    //  *
    //  * @return true if initialization was successful
     
    bool Initialize(uint8_t data_size_select, FrameModes format, uint8_t divide, SPI_Port port);

    /**
     * Transfers a byte via SSP to an external device using the SSP data register.
     * This region must be protected by a mutex static to this class.
     *
     * @return received byte from external device via SSP data register.
     */
    uint8_t Transfer(uint8_t send);
    
 
 private:
    LPC_SSP_TypeDef* LPC_SSPx;
};
#endif