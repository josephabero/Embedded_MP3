#include <FreeRTOS.h>

#include "source/gpio.hpp"
#include "source/spi.hpp"
#include "source/AT25SF041.hpp"

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

static volatile xQueueHandle button_queue_handle;

namespace SpiLab
{
  void SpiTask(void *task_parameter) {
    sjsu::LogInfo("Initializing SPI Task...");

    sjsu::LogInfo(" Initializing AT25SF041 Flash...");
    SpiLab::Gpio CS(1, 10);
    SpiLab::AT25SF041 flash_mem(&CS);
    flash_mem.Initialize();

    while(true) {
      std::array<uint32_t, 3> result = {};
      sjsu::LogInfo(" Attempting to Transfer via SPI...");
      flash_mem.CommandGetManufacturerAndDeviceId(result);

      // Log Result
      sjsu::LogInfo(" Manufacturer ID: 0x%02x", result[1]);
      sjsu::LogInfo("       Device ID: 0x%02x", result[2]);

      vTaskDelay(10000);
    }
  }
} // namespace SpiLab

int main()
{
  sjsu::LogInfo("Starting Application...");
  sjsu::Delay(1s);

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
