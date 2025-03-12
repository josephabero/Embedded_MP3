#include <FreeRTOS.h>

#include "source/gpio.hpp"

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

static volatile SemaphoreHandle_t button_press;

namespace GpioLab
{
void button_ISR() {
  xSemaphoreGive(button_press);
}

void wait_task(void *task_parameter) {
  sjsu::LogInfo("Initializing Task to Wait...");
  GpioLab::Gpio* led = static_cast<Gpio*>(task_parameter);

  while(true) {
    if(xSemaphoreTake(button_press, portMAX_DELAY)) {
      sjsu::LogInfo("Received Button Press. Toggling LED...");
      led->toggle();
    }
  }
}
} // namespace GpioLab

int main()
{
  sjsu::LogInfo("Starting Application...");
  sjsu::Delay(1s);

  vSemaphoreCreateBinary(button_press);

  static GpioLab::Gpio led(1, 18);
  static GpioLab::Gpio button(0, 29);

  led.set_direction(GpioLab::Gpio::Direction::kOutput);
  led.set_state(GpioLab::Gpio::State::kLow); // Initialize LED to Off

  button.set_direction(GpioLab::Gpio::Direction::kInput);

  button.EnableInterrupts();
  button.AttachInterruptHandler(GpioLab::button_ISR, GpioLab::Gpio::Edge::kRising);

  xTaskCreate(GpioLab::wait_task,
              "WaitTask",
              sjsu::rtos::StackSize(1024),
              &led,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);

  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
