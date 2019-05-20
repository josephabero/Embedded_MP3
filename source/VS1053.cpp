#include "LabGPIO.hpp"
#include "LabSPI.hpp"
#include "VS1053.hpp"
#include "utility/time.hpp"
#include "ff.h"

#include <cstring>
#include <cstdio>

VS1053::VS1053(LabGPIO* xdcs, LabGPIO* xcs, LabGPIO* rst, LabGPIO* dreq)
{
	XDCS = xdcs;
	XCS = xcs;
	RST = rst;
	DREQ = dreq;
}

bool VS1053::init()
{
	bool status;
  	if((XDCS == NULL) || (XCS == NULL) || (RST == NULL) || (DREQ == NULL))
  	{
  	  	status = false;
  	}
  	else
  	{
	   	XDCS->SetAsOutput();
    	XCS->SetAsOutput();
    	RST->SetAsOutput();
    	DREQ->SetAsInput();

    	XDCS->SetHigh();
    	XCS->SetHigh();
    	RST->SetHigh();

    	SPI.Initialize(8, LabSpi::FrameModes::kSPI, 48, LabSpi::SPI_Port::kPort1);
    	sciWrite(SCI_REG::kMODE, 0x4800);
    	sciWrite(SCI_REG::kCLOCKF, 0x6000);

    	status = true;
  	}
  	return status;
}

void VS1053::sciWrite(uint8_t address, uint16_t data)
{
	while(DREQ->ReadBool() != 1);
	XCS->SetLow();
	SPI.Transfer(kWrite);
	SPI.Transfer(address);		
	SPI.Transfer(data >> 8);	// Send upper 8 bits
	SPI.Transfer(data & 0xFF);	// Send lower 8 bits
	XCS->SetHigh();
}

uint16_t VS1053::sciRead(uint8_t address)
{
	uint16_t read_data;

	while(DREQ->ReadBool() != 1);
	XCS->SetLow();
	SPI.Transfer(kRead);
	SPI.Transfer(address);

	read_data = SPI.Transfer(0xFF);
	read_data = read_data << 8;
	read_data |= SPI.Transfer(0xFF);
	XCS->SetHigh();

	return read_data;
}

void VS1053::playSong(char * song_name)
{
    readFile(song_name);
}

void VS1053::readFile(char * song_name)
{
    char full_song_path[100];
    FIL file;
    size_t file_size;
    size_t bytes_read;

    size_t total_read = 0;
    bool read_file = false;
    uint8_t buffer[512] = {0};

    printf("\nsong: %s\n", song_name);
    snprintf(full_song_path, sizeof(full_song_path), "/%s", song_name);
    printf("f_open: %i\n", f_open(&file, full_song_path, FA_READ));
    file_size = f_size(&file);
    printf("file name: %s  file size: %i", full_song_path, file_size);

    while(total_read < file_size)
    {
        if(!read_file)
        {
            f_read(&file, buffer, sizeof(buffer), &bytes_read);
            // printf("total_read: %i bytes_read: %i\n", total_read, bytes_read);
            total_read += bytes_read;
            read_file = true;
        }
        if(DREQ->ReadBool())
        {
            SendData(buffer, sizeof(buffer));
            read_file = false;
        }
    }
    f_close(&file);
}

void VS1053::SendData(uint8_t* buffer, uint16_t buffer_size)
{
    // printf("\nTrying to send\n");
    while(!(DREQ->ReadBool()));
    XDCS->SetLow();
    for(uint16_t i = 0; i < buffer_size; i++)
    {
        // check DREQ every 32 bytes
        if((i % 32) == 0)
        {
            while(!(DREQ->ReadBool()));
        }
        // printf("buffer data: %i\n", *buffer);
        SPI.Transfer(*buffer++);
    }
    XDCS->SetHigh();
}

void VS1053::setVolume(uint8_t vol)
{
    uint16_t volume = (vol << 8) | vol;
    sciWrite(SCI_REG::kVOLUME, volume);
}

void VS1053::setTreble(uint8_t amplitude, uint8_t freq)
{
    bass_reg.treble_amp = amplitude;
    bass_reg.treble_freq = freq;
    sciWrite(SCI_REG::kBASS, bass_reg.word);
}

void VS1053::setBass(uint8_t amplitude, uint8_t freq)
{   
    bass_reg.bass_amp = amplitude;
    bass_reg.bass_freq = freq;
    sciWrite(SCI_REG::kBASS, bass_reg.word);
}

void VS1053::sineTest(uint8_t frequency)
{
	XCS->SetLow(); 
    SPI.Transfer(kWrite); 
    SPI.Transfer(kMODE); 
    SPI.Transfer(0x08); 
    SPI.Transfer(0x24);
    XCS->SetHigh();

    Delay(5);

    while(!DREQ->ReadBool());

    Delay(5); 

    XDCS->SetLow();
    SPI.Transfer(0x53); 
    SPI.Transfer(0xef); 
    SPI.Transfer(0x6e); 
    SPI.Transfer(frequency); 
    SPI.Transfer(0x00); 
    SPI.Transfer(0x00); 
    SPI.Transfer(0x00); 
    SPI.Transfer(0x00);
    XDCS->SetHigh();

    Delay(2000); 

    XDCS->SetLow();
    SPI.Transfer(0x45); 
    SPI.Transfer(0x78); 
    SPI.Transfer(0x69); 
    SPI.Transfer(0x74); 
    SPI.Transfer(0x00); 
    SPI.Transfer(0x00); 
    SPI.Transfer(0x00); 
    SPI.Transfer(0x00);
    XDCS->SetHigh();
}