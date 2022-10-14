#include <FreeRTOS.h>

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"

namespace
{
void task_one([[maybe_unused]] void * task_parameter) {
  sjsu::LogInfo("Initializing Task 1...");
  while(true) {
      fprintf(stderr, "AAAAAAAAAAAA");

      vTaskDelay(500);
  }
}

void task_two([[maybe_unused]] void * task_parameter) {
  sjsu::LogInfo("Initializing Task 2...");
  while(true) {
      fprintf(stderr, "bbbbbbbbbbbb");

      vTaskDelay(500);
  }
}
}  // namespace

int main()
{
  sjsu::LogInfo("Starting Hello World Application");
  sjsu::Delay(1s);
  
  xTaskCreate(task_one, // Make function LedToggle a task
              "Task1",  // Give this task the name "LedToggleTask"
              sjsu::rtos::StackSize(1024),     // Size of stack allocated to task
              sjsu::rtos::kNoParameter,  // Parameter to be passed to task
              sjsu::rtos::Priority::kLow,      // Give this task low priority
              sjsu::rtos::kNoHandle);          // Optional reference to the task
  xTaskCreate(task_two, // Make function ButtonReader a task
              "Task2",  // Give this task the name "ButtonReaderTask"
              sjsu::rtos::StackSize(1024),    // Size of stack allocated to task
              sjsu::rtos::kNoParameter,       // Pass nothing to this task
              sjsu::rtos::Priority::kLow,  // Give this task medium priority
              sjsu::rtos::kNoHandle);         // Do not supply a task handle
  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
