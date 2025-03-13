#pragma once 

#include <cstdint>
  
#include <stdint.h>
#include <stdbool.h>

#include "utility/log.hpp"
#include "peripherals/cortex/interrupt.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

typedef void (*IsrPointer)();

namespace GpioLab
{
class Gpio {
 public:
  enum class Direction : uint8_t
  {
    kInput  = 0,
    kOutput = 1
  };

  enum class State : uint8_t
  {
    kLow  = 0,
    kHigh = 1
  };

  enum class Edge
  {
    kNone = 0,
    kRising,
    kFalling,
    kBoth
  };

  static constexpr size_t kPorts = 2;
  static constexpr size_t kPins = 32;

  /// You should not modify any hardware registers at this point
  /// You should store the port and pin using the constructor.
  ///
  /// @param port - port number between 0 and 5
  /// @param pin - pin number between 0 and 32
  Gpio(uint8_t port, uint8_t pin){
    // Determine Pin and Port
    switch(port){
      case 0:
        IOxIntEnR = &sjsu::lpc40xx::LPC_GPIOINT->IO0IntEnR;
        IOxIntEnF = &sjsu::lpc40xx::LPC_GPIOINT->IO0IntEnF;
        break;
      case 1:
            break;
      case 2:
        IOxIntEnR = &sjsu::lpc40xx::LPC_GPIOINT->IO2IntEnR;
        IOxIntEnF = &sjsu::lpc40xx::LPC_GPIOINT->IO2IntEnF;
        break;
      case 3:
      case 4:
      case 5:
          break;
      default:
        sjsu::LogError("Invalid GPIO port: %d", port);
        exit(1);
        break;
    }

    constexpr uint8_t max_pin_num = 32;
    if(pin > max_pin_num) {
      sjsu::LogError("Couldn't initialize GPIO %d.%d! Invalid pin number '%d'.", port, pin, pin);
      return;
    }

    port_ = port;
    pin_ = pin;
    gpio_ = LPC_GPIO[port];

    set_as_output();
    set_high();
  }

  // @param isr  - function to run when the interrupt event occurs.
  // @param edge - condition for the interrupt to occur on.
  void AttachInterruptHandler(IsrPointer isr, Edge edge) {
    pin_isr_map[port_][pin_] = isr;
    switch(edge){
      case Edge::kRising:
        *IOxIntEnR |= 1 << pin_;
        *IOxIntEnF &= ~(1 << pin_);
        break;
      case Edge::kFalling:
        *IOxIntEnR &= ~(1 << pin_);
        *IOxIntEnF |= 1 << pin_;
        break;
      case Edge::kBoth:
        *IOxIntEnR |= 1 << pin_;
        *IOxIntEnF |= 1 << pin_;
        break;
      case Edge::kNone:
      default:
        *IOxIntEnR &= ~(1 << pin_);
        *IOxIntEnF &= ~(1 << pin_);
    }
  }

  // Register GPIO_IRQn here
  static void EnableInterrupts() {
    sjsu::InterruptController::GetPlatformController().Enable(sjsu::InterruptController::RegistrationInfo_t{
        .interrupt_request_number = sjsu::lpc40xx::GPIO_IRQn,
        .interrupt_handler        = GpioInterruptHandler,
    });
  }

  /// Sets this GPIO as an input
  /// @param output - true => output, false => set pin to input
  void set_direction(Direction direction) {
    switch(direction) {
      case Direction::kInput:
        set_as_input();
        break;
      case Direction::kOutput:
        set_as_output();
        break;
      default:
        sjsu::LogError("Attempted to set invalid direction '%d' for GPIO P%d.%d.", direction, port_, pin_);
    }
  }

  /// Set pin state to high or low depending on the input state parameter.
  /// Has no effect if the pin is set as "input".
  ///
  /// @param state - State::kHigh => set pin high, State::kLow => set pin low
  void set_state(State state) {
    switch(state) {
      case State::kLow:
        set_low();
        break;
      case State::kHigh:
        set_high();
        break;
      default:
        sjsu::LogError("Attempted to set invalid state '%d' for GPIO P%d.%d.", state, port_, pin_);
    }
  }

  /// Toggles level of the pin
  void toggle() {
    if(read_bool()) {
      set_state(State::kLow);
    }
    else {
      set_state(State::kHigh);
    }
  }

  /// Should return the state of the pin (input or output, doesn't matter)
  ///
  /// @return level of pin high => true, low => false
  State read() {
    if (gpio_->PIN & (1 << pin_)) {
      return State::kHigh;
    }
    return State::kLow;
  }

  /// Should return the state of the pin (input or output, doesn't matter)
  ///
  /// @return level of pin high => true, low => false
  bool read_bool() {
    if (gpio_->PIN & (1 << pin_)) {
      return true;
    }
    return false;
  }

  uint8_t get_port() { return port_; };
  uint8_t get_pin() { return pin_; };

 private:
  /// Sets this GPIO as an input
  void set_as_input() {
    gpio_->DIR &= ~(1 << pin_);
  }

  /// Sets this GPIO as an output
  void set_as_output() {
    gpio_->DIR |= (1 << pin_);
  }

  /// Set voltage of pin to HIGH
  void set_high() {
    gpio_->PIN |= (1 << pin_);
  }

  /// Set voltage of pin to LOW
  void set_low() {
    gpio_->PIN &= ~(1 << pin_);
  }

  uint8_t port_;
  uint8_t pin_;
  sjsu::lpc40xx::LPC_GPIO_TypeDef * gpio_;

  volatile uint32_t * IOxIntEnR;
  volatile uint32_t * IOxIntEnF;

  sjsu::lpc40xx::LPC_GPIO_TypeDef * LPC_GPIO[6] = {
    sjsu::lpc40xx::LPC_GPIO0,
    sjsu::lpc40xx::LPC_GPIO1,
    sjsu::lpc40xx::LPC_GPIO2,
    sjsu::lpc40xx::LPC_GPIO3,
    sjsu::lpc40xx::LPC_GPIO4,
    sjsu::lpc40xx::LPC_GPIO5
  };

  static inline IsrPointer pin_isr_map[kPorts][kPins] = { nullptr };

  static void GpioInterruptHandler(void) {
    // 1. Check status bits
    // 2. Clear interrupt
    // 3. Lookup and call callback from pin_isr_map

    uint32_t IOxIntStatR;
    uint32_t IOxIntStatF;
    volatile uint32_t *IOxIntClr;
    uint8_t port;

    // Port 0
    if(sjsu::lpc40xx::LPC_GPIOINT->IntStatus & 1)                  // Check bit 0 for Port 0 Interrupt Status
    {
      IOxIntStatR = sjsu::lpc40xx::LPC_GPIOINT->IO0IntStatR;
      IOxIntStatF = sjsu::lpc40xx::LPC_GPIOINT->IO0IntStatF;
      IOxIntClr = &sjsu::lpc40xx::LPC_GPIOINT->IO0IntClr;
      port = 0;
    }
    else if((sjsu::lpc40xx::LPC_GPIOINT->IntStatus >> 2) & 1)      // Check bit 2 for Port 2 Interrupt Status
    {
      IOxIntStatR = sjsu::lpc40xx::LPC_GPIOINT->IO2IntStatR;
      IOxIntStatF = sjsu::lpc40xx::LPC_GPIOINT->IO2IntStatF;
      IOxIntClr = &sjsu::lpc40xx::LPC_GPIOINT->IO2IntClr;
      port = 2;
    }

    for(uint8_t i = 0; i < 32; i++)
    {
      if((IOxIntStatR >> i) & 1)
      {
          // LOG_INFO("Rising Edge.");
          pin_isr_map[port][i]();
          *IOxIntClr |= (1 << i);
          break;
      }
      if((IOxIntStatF >> i) & 1)
      {
          // LOG_INFO("Falling Edge.");
          pin_isr_map[port][i]();
          *IOxIntClr |= (1 << i);
          break;
      }
    }
  }
};
}  // namespace GpioLab

