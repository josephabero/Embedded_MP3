#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

static void task_one(void *task_parameter);
static void task_two(void *task_parameter);

int main(void) {
  xTaskCreate(task_one, "task_one", 1024, NULL, PRIORITY_LOW, NULL);

  xTaskCreate(task_two, "task_two", 1024, NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler();

  return 0;
}

static void task_one(void *task_parameter) {
  while (true) {
    fprintf(stderr, "AAAAAAAAAAAA");

    // Sleep for 100ms
    vTaskDelay(100);
  }
}

static void task_two(void *task_parameter) {
  while (true) {
    fprintf(stderr, "bbbbbbbbbbbb");
    vTaskDelay(100);
  }
}
