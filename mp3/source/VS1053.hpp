#pragma once

#include "LabGPIO.hpp"
#include "LabSPI.hpp"

class VS1053 {
    public:
        VS1053(LabGPIO* data, LabGPIO* select, LabGPIO* reset, LabGPIO* dreq);
        bool init();

        void playSong(char * song_name);
        void SendData(uint8_t* buffer, uint16_t buffer_size);

        void setVolume(uint8_t vol);
        void setBass(uint8_t amplitude, uint8_t freq);
        void setTreble(uint8_t amplitude, uint8_t freq);
        

        void sineTest(uint8_t frequency);
    private:
        enum INSTRUCTION : uint8_t
        {
            kWrite = 0x02,
            kRead = 0x03
        };

        enum SCI_REG
        {
            kMODE       = 0x0,
            kSTATUS     = 0x1,
            kBASS       = 0x2,
            kCLOCKF     = 0x3,
            kDECODETIME = 0x4,
            kAUDATA     = 0x5,
            kWRAM       = 0x6,
            kWRAMADDR   = 0x7,
            kAIADDR     = 0xA,
            kVOLUME     = 0xB
        };

        typedef union 
        {   
            uint16_t word;
            struct
            {
                uint8_t bass_freq   : 4;
                uint8_t bass_amp    : 4;
                uint8_t treble_freq : 4;
                uint8_t treble_amp  : 4;
            }__attribute__((packed));
        } bassReg;

        void readFile(char * song_name);
       

        uint16_t sciRead(uint8_t address);
        void sciWrite(uint8_t address, uint16_t data);

        bassReg bass_reg;

        LabGPIO* XDCS; // Find SPI pin to use
        LabGPIO* XCS; // Find SPI pin to use
        LabGPIO* DREQ;
        LabGPIO* RST;
        LabSpi SPI; 
};