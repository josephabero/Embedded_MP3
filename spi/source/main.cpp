#include <FreeRTOS.h>

#include "source/gpio.hpp"
#include "source/spi.hpp"
// #include "source/AT25SF041.hpp"

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

static volatile xQueueHandle button_queue_handle;

namespace SpiLab
{
  void SpiTask(void *task_parameter) {
    sjsu::LogInfo("Initializing SPI Task...");
    SpiLab::Gpio CS(1, 10);
    SpiLab::Spi spi;

    sjsu::LogInfo("Initializing AT25SF041 Flash...");
    CS.SetAsOutput();
    CS.SetHigh();       // Initialize CS to Disabled

    const uint8_t frame_data_size = 8;
    const uint8_t clock_divider = 48;

    // SpiLab::AT25SF041 flash_mem(&CS);
    spi.Initialize(SpiLab::Spi::SpiPort::kPort2,
                   frame_data_size,
                   SpiLab::Spi::FrameFormat::kSPI,
                   clock_divider,
                   SpiLab::Spi::MasterSlaveMode::kMaster);


    // flash_mem.Initialize();
    uint32_t result[3] = { };

    while(true) {
      sjsu::LogInfo("Attempting to Transfer via SPI...");
      // flash_mem.SendCommand(SpiLab::AT25SF041::Commands::kManufacturerAndDeviceId, result);
      CS.SetLow();
      result[0] = spi.Transfer(0x9F);
      result[1] = spi.Transfer(0x00);
      result[2] = spi.Transfer(0x00);
      CS.SetHigh();
      sjsu::LogInfo("Manufacturer ID: 0x%02x", result[1]);
      sjsu::LogInfo("      Device ID: 0x%02x", result[2]);

      vTaskDelay(10000);
    }
  }
} // namespace SpiLab

int main()
{
  sjsu::LogInfo("Starting Application...");
  sjsu::Delay(1s);

  // button_queue_handle = xQueueCreate(2, sizeof(std::tuple<uint8_t, uint8_t>));


  xTaskCreate(SpiLab::SpiTask,
              "SpiTask",
              sjsu::rtos::StackSize(1024),
              nullptr,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);

  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
