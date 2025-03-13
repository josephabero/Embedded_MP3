#include <FreeRTOS.h>

#include "source/gpio.hpp"

#include "utility/log.hpp"
#include "utility/rtos/freertos/rtos.hpp"
#include "utility/time/time.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

namespace GpioLab
{

GpioLab::Gpio led1(1, 18);
GpioLab::Gpio button1(0, 29);
GpioLab::Gpio button2(0, 30);

constexpr std::chrono::nanoseconds DEBOUNCE_TIME = 10ms; // Debounce Delay

static const uint8_t NUM_OF_BUTTONS = 2;
static volatile xQueueHandle button_queue_handle;

static constexpr GpioLab::Gpio* buttons[NUM_OF_BUTTONS] = {
    &button1,
    &button2,
  };

GpioLab::Gpio* GetGpioForInt(){
  uint8_t pin = 0;
  uint32_t IOxIntStatR;
  uint32_t IOxIntStatF;

  for(uint8_t i = 0; i < NUM_OF_BUTTONS; i++)
  {
    pin = buttons[i]->get_pin();

    // Port 0
    if(sjsu::lpc40xx::LPC_GPIOINT->IntStatus & 1)                  // Check bit 0 for Port 0 Interrupt Status
    {
      IOxIntStatR = sjsu::lpc40xx::LPC_GPIOINT->IO0IntStatR;
      IOxIntStatF = sjsu::lpc40xx::LPC_GPIOINT->IO0IntStatF;
    }
    else if((sjsu::lpc40xx::LPC_GPIOINT->IntStatus >> 2) & 1)      // Check bit 2 for Port 2 Interrupt Status
    {
      IOxIntStatR = sjsu::lpc40xx::LPC_GPIOINT->IO2IntStatR;
      IOxIntStatF = sjsu::lpc40xx::LPC_GPIOINT->IO2IntStatF;
    }

    if(((IOxIntStatR >> pin) & 1) or ((IOxIntStatF >> pin) & 1)){
      return buttons[i];
    }
  }
  return nullptr;
}

void ButtonIsr() {
  std::chrono::nanoseconds time_delta;
  static std::chrono::nanoseconds start_time;
  static std::chrono::nanoseconds prev_sent_time;

  GpioLab::Gpio* button = GpioLab::GetGpioForInt();

  start_time = sjsu::Uptime();
  time_delta = start_time - prev_sent_time;
  if(time_delta > DEBOUNCE_TIME){
    xQueueSendFromISR(button_queue_handle, button, 0);
  }
  prev_sent_time = start_time;
}

void Initialize(){
  led1.set_direction(GpioLab::Gpio::Direction::kOutput);
  led1.set_state(GpioLab::Gpio::State::kLow); // Initialize LED to Off

  button1.set_direction(GpioLab::Gpio::Direction::kInput);
  button2.set_direction(GpioLab::Gpio::Direction::kInput);

  button1.EnableInterrupts();
  button1.AttachInterruptHandler(GpioLab::ButtonIsr, GpioLab::Gpio::Edge::kRising);
  button2.AttachInterruptHandler(GpioLab::ButtonIsr, GpioLab::Gpio::Edge::kRising);

  button_queue_handle = xQueueCreate(2, sizeof(std::tuple<uint8_t, uint8_t>));
}

void WaitTask(void *task_parameter) {
  sjsu::LogInfo("Initializing Task to Wait...");
  GpioLab::Gpio* led = static_cast<Gpio*>(task_parameter);
  GpioLab::Gpio gpio = button1;

  while(true) {
    if(xQueueReceive(button_queue_handle, &gpio, portMAX_DELAY)) {
      sjsu::LogInfo("Received Button Press. Toggling LED...");
      sjsu::LogInfo("Received Button Press for P%d.%d. Toggling LED...", gpio.get_port(), gpio.get_pin());
      led->toggle();
    }
  }
}
} // namespace GpioLab

int main()
{
  sjsu::LogInfo("Starting Application...");
  sjsu::Delay(1s);

  GpioLab::Initialize();

  xTaskCreate(GpioLab::WaitTask,
              "WaitTask",
              sjsu::rtos::StackSize(1024),
              &GpioLab::led1,
              sjsu::rtos::Priority::kLow,
              sjsu::rtos::kNoHandle);

  sjsu::LogInfo("Starting Scheduler...");
  vTaskStartScheduler();

  return 0;
}
