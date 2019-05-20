#include "LabSpi.hpp"
#include <stdio.h>
#include "utility/log.hpp"
bool LabSpi::Initialize(uint8_t data_size_select, FrameModes format, uint8_t divide, SPI_Port port) 
{
     // Initialize SSP peripheral
    // 1. Power (PCONP register. set PCSSP2!)
    // 2. Set Peripheral Clock
    // 3. Set SSP pins
    // IOCON_P1_0 -> 100 for SSP2_SCK
    // IOCON_P1_1 -> 100 for SSP2_MOSI
    // IOCON_P1_4 -> 100 for SSP2_MISO
    if (divide < 2 || divide & (1 << 0)) //check if divide is 0 or 1 or if even number by checking LSB
    {
        LOG_WARNING("SSP interface FAILED to initialize: Value of divisor is invalid! Must be an even number between 2 and 254.");
        return false;
    }


    if(port == kPort1)
    {
        LOG_INFO("Enable SSP1");
        LOG_INFO("Initializing P0.7 as SSP1_SCK");
        LOG_INFO("Initializing P0.9 as SSP1_MOSI");
        LOG_INFO("Initializing P0.8 as SSP1_MISO");

        LPC_SC->PCONP |=  (1 << 20);                            // Enable power for SSP1 (PCSSP1)
        LPC_IOCON->P0_7 = (LPC_IOCON->P0_7 & ~(0b111)) | 0b010; // Sets P0_7 as SSP1_SCK
        LPC_IOCON->P0_9 = (LPC_IOCON->P0_9 & ~(0b111)) | 0b010; // Sets P0_9 as SSP1_MOSI
        LPC_IOCON->P0_8 = (LPC_IOCON->P0_8 & ~(0b111)) | 0b010; // Sets P0_8 as SSP1_MISO

        LPC_SSPx = LPC_SSP1;
    }
    else if(port == kPort2)
    {
        LOG_INFO("Enable SSP2");
        LOG_INFO("Initializing P1.0 as SSP2_SCK");
        LOG_INFO("Initializing P1.1 as SSP2_MOSI");
        LOG_INFO("Initializing P1.4 as SSP2_MISO");

        LPC_SC->PCONP |=  (1 << 20);                            // Enable power for SSP2 (PCSSP2)
        LPC_IOCON->P1_0 = (LPC_IOCON->P1_0 & ~(0b111)) | 0b100; // Sets P1_0 as SSP2_SCK
        LPC_IOCON->P1_1 = (LPC_IOCON->P1_1 & ~(0b111)) | 0b100; // Sets P1_1 as SSP2_MOSI
        LPC_IOCON->P1_4 = (LPC_IOCON->P1_4 & ~(0b111)) | 0b100; // Sets P1_4 as SSP2_MISO

        LPC_SSPx = LPC_SSP2;
    }
    else
    {
        LOG_WARNING("SSP interface FAILED to initialize: Invalid Port is used.");
        return false;
    }

    
    
    // Initially clear CR0
    LPC_SSPx->CR0 = 0;         
    LPC_SSPx->CR0 |= (data_size_select - 1);    // Set DSS to user defined bit mode
    LPC_SSPx->CR0 |= format;                    // Sets format based on user definition
    LPC_SSPx->CR0 &= ~(1 << 6);                 // Clear CPOL, bus clock low between frames
    LPC_SSPx->CR0 &= ~(1 << 7);                 // Clear CPHA, first transition
    LPC_SSPx->CR0 &= ~(0xFFFF << 8);            // Clear SCR to 0  
    LPC_SSPx->CPSR |= divide;                   // Set CPSR to user divide, ONLY ALLOWS FOR EVEN NUMBERS TO DIVIDE

    // Set CR1
    LPC_SSPx->CR1 &= ~(1);           // Set to normal operation
    LPC_SSPx->CR1 &= ~(1 << 2);      // Set SSP2 as Master
    LPC_SSPx->CR1 |= (1 << 1);       // Enable SSP2

    return true;
}

uint8_t LabSpi::Transfer(uint8_t send) 
{
    uint8_t result_byte = 0;

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