#include <FreeRTOS.h>

#include "gpio/source/gpio.hpp"

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

static SemaphoreHandle_t button_press;

namespace GpioLab
{
void toggle_led_task(void *task_parameter) {
  sjsu::LogInfo("Initializing LED Task...");

  GpioLab::Gpio* led = static_cast<Gpio*>(task_parameter);
  sjsu::LogInfo("Initialized LED %d_%d...", led->get_port(), led->get_pin());

  sjsu::LogInfo("Setting LED as Output...");
  led->set_as_output();
  led->set_high();  // Initialize LED to be Off

  uint32_t timeout = 5000;

  while (true) {
    if (xSemaphoreTake(button_press, timeout)) {
      sjsu::LogInfo("Toggling LED...");
      led->toggle();
    }
    else {
      sjsu::LogInfo("LED task timed out after waiting %dms for button press...",
                    timeout);
    }
    vTaskDelay(50);
  }
}

void button_press_task(void *task_parameter) {
  sjsu::LogInfo("Initializing Button Task...");

  GpioLab::Gpio* button = static_cast<Gpio*>(task_parameter);
  sjsu::LogInfo("Initialized Button %d_%d...",
                button->get_port(),
                button->get_pin());

  sjsu::LogInfo("Setting Button as Input...");
  button->set_as_input();

  uint8_t previous_state = GpioLab::Gpio::Level::kHigh;
  uint8_t current_state = button->get_state();

  while (true) {
    current_state = button->get_state();

    // Release Semaphore when Button is released
    if (previous_state == GpioLab::Gpio::Level::kHigh &&
        current_state == GpioLab::Gpio::Level::kLow) {
      xSemaphoreGive(button_press);
    }
    previous_state = current_state;
    vTaskDelay(50);
  }
}
}  // namespace GpioLab

int main()
{
  sjsu::LogInfo("Starting Application...");
  sjsu::Delay(1s);

  vSemaphoreCreateBinary(button_press);

  static GpioLab::Gpio led(1, 18);
  static GpioLab::Gpio button(0, 29);

  xTaskCreate(GpioLab::toggle_led_task,
              "LED Task",
              sjsu::rtos::StackSize(1024),
              &led,
              sjsu::rtos::Priority::kHigh,
              sjsu::rtos::kNoHandle);
  xTaskCreate(GpioLab::button_press_task,
              "Button Task",
              sjsu::rtos::StackSize(1024),
              &button,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);

  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
