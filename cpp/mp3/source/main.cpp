#include <project_config.hpp>
#include <FreeRTOS.h>

#include "../../library/third_party/fatfs/source/ff.h"
#include "event_groups.h"
#include "L0_LowLevel/interrupt.hpp"
#include "L3_Application/commandline.hpp"
#include "L3_Application/commands/common.hpp"
#include "L3_Application/commands/lpc_system_command.hpp"
#include "L3_Application/commands/rtos_command.hpp"
#include "L3_Application/oled_terminal.hpp"
#include "LabGPIO.hpp"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "third_party/fatfs/source/ff.h"
#include "third_party/FreeRTOS/Source/include/semphr.h"
#include "third_party/FreeRTOS/Source/include/task.h"
#include "utility/log.hpp"
#include "utility/rtos.hpp"
#include "utility/time.hpp"
#include "VS1053.hpp"
#include <cinttypes>
#include <iterator>


const uint32_t STACK_SIZE = 512;
#define START_TIME 0
#define END_TIME 1

// #define DEBOUNCE_TIME_MS 1 * 1000 * 1000ULL
// #define DEBOUNCE_TIME_MS 25 * 1000ULL
#define DEBOUNCE_TIME_1MS 100 * 1000ULL // 1 MS Delay



// --------------- S T R U C T S ------------------------
typedef union
{
    uint8_t buffer[128];
    struct 
    {
        uint8_t header[3];
        uint8_t title[30];
        uint8_t artist[30];
        uint8_t album[30];
        uint8_t year[4];
        uint8_t comment[28];
        uint8_t zero;
        uint8_t track;
        uint8_t genre;
    } __attribute__((packed));
} ID3v1_t;

typedef struct SettingsCommand
{
    uint8_t type;
    uint8_t value;
};


// ------------- E N U M S --------------
enum Menu
{
    kSongInfo = 0,
    kSongList,
    kSettings,
    kMenuMaxSize
};

enum CommandValues
{
    kVolumeCommand = 0,
    kTrebleCommand,
    kBassCommand,
    kSongCommand
};

enum Constants
{
    kVolumeMin = 0,
    kVolumeMax = 10,
    kFirstSong = 0,
    kLastSong = 7,
    kSongCount = 8,
    kCursorPositionMin = 0,
    kCursorPositionMax = 7,
    kTrebleMin = 0,
    kTrebleMax = 10,
    kBassMin = 0,
    kBassMax = 10
};

enum IrOpcode
{
    kPower        = 0x0778,
    kSource       = 0x5728,
    kVolumeUp     = 0x7708,
    kVolumeDown   = 0x0F70,
    kMute         = 0x4738,
    kSelectSong   = 0x48B7,
    kPrevious     = 0x6897,
    kPlayPause    = 0x28D7,
    kNext         = 0x18E7,
    kSoundEffect  = 0x6F10,
    kSound        = 0x40BF,
    kBluetooth    = 0x5CA3,
    kLeft         = 0x06F9,
    kRight        = 0x46B9,
    kSoundControl = 0x32CD
};

// ---------- G L O B A L   V A R I A B L E S ---------------
LabGPIO IR(0, 18);
LabGPIO BUTTON1(0, 18);
LabGPIO BUTTON2(0, 15);

LabGPIO XDCS(1, 30);
LabGPIO XCS(1, 14);
LabGPIO XRST(0, 25);
LabGPIO DREQ(1, 23);

VS1053 Decoder(&XDCS, &XCS, &XRST, &DREQ);
OledTerminal oled;

uint8_t volume_level = kVolumeMin;
uint8_t treble_level = kTrebleMin;
uint8_t bass_level = kBassMin;

uint8_t menu_index = 0;
uint8_t song_index = 0; // current song index

uint8_t cursor_position = 0;
uint8_t songNumber = 18;
uint8_t pages = songNumber / 8;
uint8_t current_page = 0;

uint8_t song_count;
ID3v1_t mp3_files[100];
char fileName[100][256];

uint32_t file_size = 0;
uint32_t total_bytes_read = 0;

FATFS fs;
FIL song_file;

bool play_pause = true;
bool treble_bass = true;
bool mute = false;

CommandList_t<32> command_list;
RtosCommand rtos_command;
CommandLine<command_list> ci;

QueueHandle_t decoderQueueHandle;
xQueueHandle irRemoteQueueHandle;
xQueueHandle settingsCommandQueueHandle;

TaskHandle_t prod;

SemaphoreHandle_t SPI_MUTEX = NULL;
SemaphoreHandle_t SD_MUTEX = NULL;


// ------------- C O N S T A N T S ---------------
char STATUS[11][17] = {
    "   *****|       ",
    "    ****|       ",
    "     ***|       ",
    "      **|       ",
    "       *|       ",
    "        |       ",
    "        |*      ",
    "        |**     ",
    "        |***    ",
    "        |****   ",
    "        |*****  "
};

void readIR_ISR();

void button1ISR();
void button2ISR();

void printSongList();
void vIrRemoteTask(void * pvParameter);
void vSettingsTask(void * pvParameter);
volatile uint32_t low = 0;


// ------------- F U N C  D E C L A R A T I O N S  ---------------
void MP3Init();
void printMetaData(ID3v1_t mp3);
void ReadSDCard(char* path, uint8_t* file_count);




// RTOS COMMAND constants for writing to CPU.TXT
static constexpr const char kDescription[] =
    "Display FreeRTOS runtime stats.";
static constexpr const char kDivider[] =
    "+------------------+-----------+------+------------+-------------+";
static constexpr const char kHeader[] =
    "|    Task Name     |   State   | CPU% | Stack Left |   Priority  |\r\n"
    "|                  |           |      |  in words  | Base : Curr |";

static const char * RtosStateToString(eTaskState state)
{
    switch (state)
    {
        case eTaskState::eBlocked: return "BLOCKED";
        case eTaskState::eDeleted: return "DELETED";
        case eTaskState::eInvalid: return "INVALID";
        case eTaskState::eReady: return "READY";
        case eTaskState::eRunning: return "RUNNING";
        case eTaskState::eSuspended: return "SUSPENDED";
        default: return "UNKNOWN";
    }
}



// ------------- T A S K S  ---------------
void vDecoderProducerTask(void *p);
void vDecoderConsumerTask(void *p);


void vFileTask(void *p);
void vTerminalTask(void *p);



    
int main(void)
{
    MP3Init();

    SPI_MUTEX = xSemaphoreCreateMutex();
    SD_MUTEX = xSemaphoreCreateMutex();

    // // OPTIMIZE WITH TYPEDEF OR SOMETHING
    uint8_t chunk_buffer[512] = {0};
    decoderQueueHandle = xQueueCreate(2, sizeof(chunk_buffer));

    LOG_INFO("Starting IR Application. . . .");
    irRemoteQueueHandle = xQueueCreate(10, sizeof(uint16_t));
    settingsCommandQueueHandle = xQueueCreate(10, sizeof(SettingsCommand));

    xTaskCreate(
            vDecoderProducerTask, 
            "PRODUCER", 
            2048, 
            NULL, 
            2, 
            &prod
        );
    xTaskCreate(
            vDecoderConsumerTask, 
            "CONSUMER", 
            1024, 
            NULL, 
            3, 
            NULL
        );

    
    xTaskCreate(
            vSettingsTask, 
            "SETTINGS", 
            1024, 
            NULL, 
            2, 
            NULL
        );
    xTaskCreate(
            vTerminalTask,
            "TERMINAL",
            1024,
            NULL,
            1,
            NULL
        );
    xTaskCreate(
            vIrRemoteTask,           /* Function that implements the task. */
            "vIrRemoteTask",         /* Text name for the task. */
            STACK_SIZE,              /* Stack size in words, not bytes. */
            NULL,                    /* Parameter passed into the task. */
            4,                       /* Priority at which the task is created. */
            NULL                     /* Used to pass out the created task's handle. */
        );                         


    vTaskStartScheduler();
  return 0;
}

void ReadSDCard(char* path, uint8_t* file_count)
{
    *file_count = 0; 

    FRESULT res;
    DIR dir;
    static FILINFO fno;
    FIL fsrc;                                               /* File object */
    BYTE buffer[4096];                                      /* File copy buffer */
    FRESULT fr;                                             /* FatFs function common result code */
    UINT br;                                                /* File read/write count */
    FSIZE_t f_rd;                                           /* File read/write pointer */
    ID3v1_t mp3_info;

    res = f_opendir(&dir, path);                            /* Open the directory */
    if (res == FR_OK) 
    {
        while (1) 
        {
            res = f_readdir(&dir, &fno);                    /* Read a directory item */
            
            if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of dir */

            if(strstr(fno.fname, ".mp3"))
            {
                /* Open and Read mp3 files */
                fr = f_open(&fsrc, fno.fname, FA_READ);
                fr = f_read(&fsrc, buffer, sizeof(buffer), &br);   /* Read a chunk of source file */
                if (fr || br == 0) break;                         /* error or eof */

                
                /* Record filenames */
                uint16_t i = 0;
                while (fno.fname[i] && i < 256) {
                    fileName[*file_count][i] = fno.fname[i];
                    i++;
                }
                fileName[*file_count][i] = '\0';

                br = 128;
                f_rd = fno.fsize - 128;
                fr = f_lseek(&fsrc, f_rd);
                if(fr) break;

                fr = f_read(&fsrc, mp3_info.buffer, sizeof(mp3_info.buffer), &br);    /* Read metadata into temp ID3v1 struct */
                if (fr || br == 0) break;                               /* error or eof */

                // printMetaData(mp3_info);
                mp3_files[*file_count] = mp3_info;   /* store ID3v1 var into mp3_file arrray */

                (*file_count)++;                /* Save total number of files on sd card */
            }
        }
        f_closedir(&dir);

        printf("File Count: %d \n",*file_count);
        int j = 0;
        for (int i = 0; i < *file_count; i++) 
        {
            printf("File Name: %s\n", fileName[i]);
        }
    }
}


void MP3Init()
{
    FRESULT res;
    char dir_path[512];
    song_count = 0;
    
    // INITIALIZE DEVICES
    LOG_INFO("Starting Decoder...");
    Decoder.init();
    Decoder.setVolume(volume_level);

    LOG_INFO("SINE TEST...");
    // Decoder.sineTest(200);

    LOG_INFO("Starting OLED...");
    oled.Initialize();
    oled.printf("                "\
                " Khalil's Kids' "\
                "      MP3       "\
                "                "\
                "                "\
                "  Press SOURCE  "\
                "    to start!   ");

    LOG_INFO("Starting Command Line Application");
    LOG_INFO("Adding common SJTwo commands to command line...");
    AddCommonCommands(ci);

    LOG_INFO("Adding rtos command to command line...");
    ci.AddCommand(&rtos_command);

    LOG_INFO("Initializing CommandLine object...");
    ci.Initialize();

    LOG_INFO("Initializing Interrupts...");
    IR.Init();
    IR.EnableInterrupts();
    IR.SetAsInput();
    IR.AttachInterruptHandler(readIR_ISR, LabGPIO::Edge::kBoth);

    LOG_INFO("Mounting SD Card...");
    res = f_mount(&fs, "", 1);
    if (res == FR_OK) {
        LOG_INFO("File System Mounted Succesfully!");
        LOG_INFO("Reading MP3 files from SD Card!");
        strcpy(dir_path, "/");
        ReadSDCard(dir_path, &song_count);
    }
    else {
        LOG_ERROR("ERROR");
        oled.printf("ERROR");
    }

    for(int i = 0; i < song_count; i++)
    {
        printf("%i. song: %s\n", i, fileName[i]);
    }

    // Open first song file for vFileTask
    f_open(&song_file, fileName[song_index], FA_READ);
}

void printMetaData(ID3v1_t mp3)
{
    printf("Header: %s\nFilename: %s\nArtist: %s\nAlbum: %s\nYear: %s\nComment: %s\nZero: %i\nTrack: %i\nGenre: %i\n", 
        mp3.header, mp3.title, mp3.artist, mp3.album, mp3.year, mp3.comment,
        mp3.zero, mp3.track, mp3.genre);
}

void vDecoderProducerTask(void *p)
{
    size_t bytes_read;
    uint8_t buffer[512] = {0};
    bool read_file = false;
    FRESULT fo;
    SettingsCommand command;

    while(1)
    {
        while(total_bytes_read < file_size)
        {
            if(!read_file)
            {
                if(xSemaphoreTake(SD_MUTEX, portMAX_DELAY))
                {
                    f_read(&song_file, buffer, sizeof(buffer), &bytes_read);
                    total_bytes_read += bytes_read;
                    read_file = true;
                    xSemaphoreGive(SD_MUTEX);
                }
            }
            if(DREQ.ReadBool())
            {
                xQueueSend(decoderQueueHandle, &buffer, portMAX_DELAY);
                vTaskDelay(45);
                read_file = false;
            }
        }

        // AUTOPLAY
        if(song_index != song_count - 1)
        {
            song_index++;
        }
        else
        {
            song_index = kFirstSong;
        }

        // Send Command For kSongCommand
        command.type = kSongCommand;
        command.value = song_index;
        xQueueSend(settingsCommandQueueHandle, &command, 0);
    }
}

void vDecoderConsumerTask(void *p)
{
    uint8_t buffer[512] = {0};
    while(1)
    {
        if(xQueueReceive(decoderQueueHandle, &buffer, portMAX_DELAY))
        {
            if(xSemaphoreTake(SPI_MUTEX, portMAX_DELAY))
            {
                Decoder.SendData(buffer, sizeof(buffer));
                xSemaphoreGive(SPI_MUTEX);
            }
        }
    }
}



void vSettingsTask(void * pvParameter)
{
    SettingsCommand command;
    while(1)
    {
        if(xQueueReceive(settingsCommandQueueHandle, &command, portMAX_DELAY))
        {
            switch(command.type)
            {
                case kVolumeCommand:
                    if(xSemaphoreTake(SPI_MUTEX, portMAX_DELAY))
                    {
                        printf("Changing volume to %d\n", command.value);
                        Decoder.setVolume(command.value);
                        xSemaphoreGive(SPI_MUTEX);
                    }
                    break;
                case kTrebleCommand:
                    if(xSemaphoreTake(SPI_MUTEX, portMAX_DELAY))
                    {
                        printf("Changing treble to %d\n", command.value);
                        Decoder.setTreble(command.value, 0x01);
                        xSemaphoreGive(SPI_MUTEX);
                    }
                    break;
                case kBassCommand:
                    if(xSemaphoreTake(SPI_MUTEX, portMAX_DELAY))
                    {
                        printf("Changing bass to %d\n", command.value);
                        Decoder.setBass(command.value, 0x01);
                        xSemaphoreGive(SPI_MUTEX);
                    }
                    break;
                case kSongCommand:
                    if(xSemaphoreTake(SD_MUTEX, portMAX_DELAY))
                    {
                        printf("Changing song to %d\n", command.value);
                        f_close(&song_file);
                        f_open(&song_file, fileName[song_index], FA_READ);
                        file_size = f_size(&song_file);
                        printf("file size: %i\n", file_size);
                        total_bytes_read = 0;
                        xSemaphoreGive(SD_MUTEX);
                    }
                    break;
            }
        }
    }
}



void vTerminalTask(void *p)
{
    LOG_INFO("Press Enter to Start Command Line!");

    ci.WaitForInput();

    LOG_WARNING("\nUser has quit from terminal!");
    vTaskDelete(nullptr);
}

void vIrRemoteTask(void * pvParameter)
{
    uint16_t opcode = 0;
    SettingsCommand command;

    while(1)
    {
        if(xQueueReceive(irRemoteQueueHandle, &opcode, portMAX_DELAY))
        {
            switch(opcode)
            {
                case IrOpcode::kSource:
                    menu_index = (menu_index + 1) % kMenuMaxSize;

                    oled.Clear();
                    switch(menu_index)
                    {
                        case kSongInfo:
                            if(play_pause)
                            {
                                oled.printf("Now Playing...\n");
                            }
                            else
                            {
                                oled.printf("Paused...     \n");
                            }
                            
                            oled.printf("Title: %s\n", mp3_files[song_index].title);
                            oled.printf("Artist: %s\n", mp3_files[song_index].artist);
                            oled.printf("Album: %s\n", mp3_files[song_index].album);
                            break;

                        case kSongList:
                            {
                                printSongList();
                                break;
                            }


                        case kSettings:
                            if(treble_bass)
                            {
                                oled.printf("****************");
                                oled.printf("*    TREBLE    *");
                                oled.printf("****************");
                                oled.SetCursor(0, 4);
                                oled.printf(STATUS[treble_level]);
                                oled.printf("  -5    0    5  ");
                            }
                            else
                            {
                                oled.printf("****************");
                                oled.printf("*     BASS     *");
                                oled.printf("****************");
                                oled.SetCursor(0, 4);
                                oled.printf(STATUS[bass_level]);
                                oled.printf("  -5    0    5  ");
                            }    
                            break;
                    }

                    break;

                case IrOpcode::kVolumeDown:
                    if(volume_level < kVolumeMax)
                    {
                        volume_level++;
                        // Send Command For kVolumeCommand
                        command.type = kVolumeCommand;
                        command.value = volume_level * 10;
                        xQueueSend(settingsCommandQueueHandle, &command, 0);
                    }

                    break;

                case IrOpcode::kVolumeUp:
                    if(volume_level > kVolumeMin)
                    {
                        volume_level--;
                        // Send Command For kVolumeCommand
                        command.type = kVolumeCommand;
                        command.value = volume_level * 10;
                        xQueueSend(settingsCommandQueueHandle, &command, 0);
                    }

                    break;

                case IrOpcode::kMute:
                    mute = !mute;

                    if(mute)
                    {
                        // Send Command For kVolumeCommand
                        command.type = kVolumeCommand;
                        command.value = 100;
                        xQueueSend(settingsCommandQueueHandle, &command, 0);
                    }
                    else
                    {
                        // Send Command For kVolumeCommand
                        command.type = kVolumeCommand;
                        command.value = volume_level;
                        xQueueSend(settingsCommandQueueHandle, &command, 0);
                    }

                    break;

                case IrOpcode::kSelectSong:
                    if(menu_index == kSongList)
                    {
                        song_index = cursor_position;
                        menu_index = kSongInfo;
                        play_pause = true;
                        vTaskResume(prod);

                        // Send Command For kSongCommand
                        command.type = kSongCommand;
                        command.value = song_index;
                        xQueueSend(settingsCommandQueueHandle, &command, 0);

                        oled.Clear();
                        if(play_pause)
                        {
                            oled.printf("Now Playing...\n");
                        }
                        else
                        {
                            oled.printf("Paused...     \n");
                        }
                        
                        oled.printf("Title: %s\n", mp3_files[song_index].title);
                        oled.printf("Artist: %s\n", mp3_files[song_index].artist);
                        oled.printf("Album: %s\n", mp3_files[song_index].album);
                    }
                    break;

                case IrOpcode::kPrevious:
                    if(song_index != kFirstSong)
                    {
                        song_index--;
                    }
                    else
                    {
                        song_index = song_count - 1;
                    }

                    play_pause = true;
                    vTaskResume(prod);

                    // Send Command For kSongCommand
                    command.type = kSongCommand;
                    command.value = song_index;
                    xQueueSend(settingsCommandQueueHandle, &command, 0);

                    if(menu_index == kSongInfo)
                    {
                        oled.Clear();
                        if(play_pause)
                        {
                            oled.printf("Now Playing...\n");
                        }
                        else
                        {
                            oled.printf("Paused...     \n");
                        }
                        
                        oled.printf("Title: %s\n", mp3_files[song_index].title);
                        oled.printf("Artist: %s\n", mp3_files[song_index].artist);
                        oled.printf("Album: %s\n", mp3_files[song_index].album);
                    }
                    break;

                case IrOpcode::kPlayPause:
                    play_pause = !play_pause;
                    if(play_pause)
                    {
                        vTaskResume(prod);
                    }
                    else
                    {
                        vTaskSuspend(prod);
                    }

                    if(menu_index == kSongInfo)
                    {
                        oled.SetCursor(0, 0);
                        if(play_pause)
                        {
                            oled.printf("Now Playing...\n");
                        }
                        else
                        {
                            oled.printf("Paused...     \n");
                        }
                    }

                    break;

                case IrOpcode::kNext:
                    if(song_index != song_count - 1)
                    {
                        song_index++;
                    }
                    else
                    {
                        song_index = kFirstSong;
                    }

                    play_pause = true;
                    vTaskResume(prod);

                    // Send Command For kSongCommand
                    command.type = kSongCommand;
                    command.value = song_index;
                    xQueueSend(settingsCommandQueueHandle, &command, 0);

                    if(menu_index == kSongInfo)
                    {
                        oled.Clear();
                        if(play_pause)
                        {
                            oled.printf("Now Playing...\n");
                        }
                        else
                        {
                            oled.printf("Paused...     \n");
                        }
                        
                        oled.printf("Title: %s\n", mp3_files[song_index].title);
                        oled.printf("Artist: %s\n", mp3_files[song_index].artist);
                        oled.printf("Album: %s\n", mp3_files[song_index].album);
                    }
                    break;

                case IrOpcode::kLeft:
                    switch(menu_index)
                    {
                        case kSongList:
                            if(cursor_position > kCursorPositionMin)
                            {
                                oled.SetCursor(0, cursor_position);
                                oled.printf(" ");

                                cursor_position--;

                                oled.SetCursor(0, cursor_position);
                                oled.printf(">");
                            }
                            else if (current_page != 0)
                            {
                                current_page--;
                                cursor_position = 0;
                                printSongList();
                            }

                            break;

                        case kSettings:
                            if(treble_bass)
                            {
                                if(treble_level > kTrebleMin)
                                {
                                    treble_level--;
                                    
                                    // Send Command For kTrebleCommand 
                                    command.type = kTrebleCommand;
                                    command.value = treble_level;
                                    xQueueSend(settingsCommandQueueHandle, &command, 0);

                                    oled.SetCursor(0, 4);
                                    oled.printf(STATUS[treble_level]);
                                }
                            }
                            else
                            {
                                if(bass_level > kBassMin)
                                {
                                    bass_level--;
                                    
                                    // Send Command For kBassCommand
                                    command.type = kBassCommand;
                                    command.value = bass_level;
                                    xQueueSend(settingsCommandQueueHandle, &command, 0);

                                    oled.SetCursor(0, 4);
                                    oled.printf(STATUS[bass_level]);
                                }
                            }
                            break;
                    }

                    break;

                case IrOpcode::kRight:
                    switch(menu_index)
                    {
                        case kSongList:
                            if(cursor_position < song_count - 1)
                            {
                                if (current_page == pages)
                                {
                                    if (cursor_position < ((songNumber % 8) - 1))
                                    {
                                        oled.SetCursor(0, cursor_position);
                                        oled.printf(" ");
                                        if(cursor_position == 0)
                                        {
                                            char title[16] = {0};
                                            memcpy(title, mp3_files[cursor_position  + (8 * current_page)].title, 15);
                                            oled.printf(title);
                                        }
                                        cursor_position++;

                                        oled.SetCursor(0, cursor_position);
                                        oled.printf(">");
                                    }
                                }
                                else
                                {
                                    oled.SetCursor(0, cursor_position);
                                    oled.printf(" ");
                                    if(cursor_position == 0)
                                    {
                                        char title[16] = {0};
                                        memcpy(title, mp3_files[cursor_position + (8 * current_page)].title, 15);
                                        oled.printf(title);
                                    }

                                    cursor_position++;

                                    oled.SetCursor(0, cursor_position);
                                    oled.printf(">");
                                }
                                
                            }
                            else if (current_page < pages)
                            {
                                current_page++;
                                LOG_INFO("current page %d", current_page);
                                cursor_position = 0;
                                printSongList();
                            }

                            break;

                        case kSettings:
                            if(treble_bass)
                            {
                                if(treble_level < kTrebleMax)
                                {
                                    treble_level++;

                                    // Send Command For kTrebleCommand
                                    command.type = kTrebleCommand;
                                    command.value = treble_level;
                                    xQueueSend(settingsCommandQueueHandle, &command, 0);

                                    oled.SetCursor(0, 4);
                                    oled.printf(STATUS[treble_level]);
                                }
                            }
                            else
                            {
                                if(bass_level < kBassMax)
                                {
                                    bass_level++;

                                    // Send Command For kBassCommand
                                    command.type = kBassCommand;
                                    command.value = bass_level;
                                    xQueueSend(settingsCommandQueueHandle, &command, 0);

                                    oled.SetCursor(0, 4);
                                    oled.printf(STATUS[bass_level]);
                                }
                            }
                            break;
                    }
                    break;

                case IrOpcode::kSoundControl:
                    if(menu_index == kSettings)
                    {
                        treble_bass = !treble_bass;
                        if(treble_bass)
                        {
                            oled.SetCursor(0, 1);
                            oled.printf("*    TREBLE    *");
                            oled.SetCursor(0, 4);
                            oled.printf(STATUS[treble_level]);
                        }
                        else
                        {
                            oled.SetCursor(0, 1);
                            oled.printf("*     BASS     *");
                            oled.SetCursor(0, 4);
                            oled.printf(STATUS[bass_level]);
                        }                        
                    }
                    break;

                default:
                    break;
            }
        }
    }
}



void readIR_ISR() 
{
    // printf("REMOTE ISR\n");
    static uint8_t sample = 0;
    static uint16_t opcode = 0;
    static uint32_t start_time = 0;
    static bool read_opcode = false;
    static uint16_t prev_opcode = 0;
    static uint64_t prev_sent_time = 0;
    if(LPC_GPIOINT->IO0IntStatR & (1 << 18))
    {
        start_time = Uptime();
    }
    else
    {
        opcode = (opcode << 1) | (((Uptime() - start_time) >> 7) == 5);
        if(opcode == 0x9E00)
        {
            sample = 0;
        }
        if(++sample == 16)
        {
            if(opcode == prev_opcode)
            {
                if (Uptime() > prev_sent_time + DEBOUNCE_TIME_1MS)
                {
                    prev_sent_time = Uptime();
                    xQueueSendFromISR(irRemoteQueueHandle, &opcode, 0);

                }
            }
            else
            {
                prev_sent_time = Uptime();
                prev_opcode = opcode;
                xQueueSendFromISR(irRemoteQueueHandle, &opcode, 0);
            }
        }
    }

}
void printSongList()
{
    oled.Clear();
    char title[16] = {0};
    //cursor_position = 0;

    oled.printf(">");
    
    for(uint8_t i = current_page * 8; i < (current_page * 8 + 8); i++)
    {
        if (current_page == pages) //last page only print remaining values
        {
            if ((i % 8) < (songNumber % 8)) //if remainder less than slots then print
            {
                memcpy(title, mp3_files[i].title, 15);
                oled.SetCursor(1, i % 8);
                oled.printf(title); 
            }
        }
        else
        {
            memcpy(title, mp3_files[i].title, 15);
            oled.SetCursor(1, i);
            oled.printf(title); 
        } 
        
    }   
}