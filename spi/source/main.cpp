#include <FreeRTOS.h>

#include "source/gpio.hpp"
#include "source/spi.hpp"

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

static volatile xQueueHandle button_queue_handle;

namespace SpiLab
{
  void WaitTask(void *task_parameter) {
    sjsu::LogInfo("Initializing Task to Wait...");
    uint8_t value = 0;

    while(true) {
      if(xQueueReceive(button_queue_handle, &value, portMAX_DELAY)) {
        sjsu::LogInfo("Received Button Press. Toggling LED...");
      }
    }
  }
} // namespace SpiLab

int main()
{
  sjsu::LogInfo("Starting Application...");
  sjsu::Delay(1s);

  button_queue_handle = xQueueCreate(2, sizeof(std::tuple<uint8_t, uint8_t>));

  SpiLab::Spi spi = SpiLab::Spi();
  spi.Initialize(SpiLab::Spi::SpiPort::kPort2, 8, SpiLab::Spi::FrameFormat::kSPI, 48, SpiLab::Spi::MasterSlaveMode::kMaster);

  xTaskCreate(SpiLab::WaitTask,
              "WaitTask",
              sjsu::rtos::StackSize(1024),
              nullptr,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);

  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
