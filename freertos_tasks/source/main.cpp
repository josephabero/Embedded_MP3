#include <FreeRTOS.h>

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"

namespace
{
void task_one([[maybe_unused]] void * task_parameter) {
  sjsu::LogInfo("Initializing Task 1...");
  while (true) {
      fprintf(stderr, "AAAAAAAAAAAA");

      vTaskDelay(500);
  }
}

void task_two([[maybe_unused]] void * task_parameter) {
  sjsu::LogInfo("Initializing Task 2...");
  while (true) {
      fprintf(stderr, "bbbbbbbbbbbb");

      vTaskDelay(500);
  }
}
}  // namespace

int main()
{
  sjsu::LogInfo("Starting Hello World Application");
  sjsu::Delay(1s);

  xTaskCreate(task_one,
              "Task1",
              sjsu::rtos::StackSize(1024),
              sjsu::rtos::kNoParameter,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);
  xTaskCreate(task_two,
              "Task2",
              sjsu::rtos::StackSize(1024),
              sjsu::rtos::kNoParameter,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);
  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
