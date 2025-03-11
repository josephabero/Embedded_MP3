#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "utility/log.hpp"
#include "platforms/targets/lpc40xx/lpc40xx.h"

namespace GpioLab
{
class Gpio {
 public:
  enum Direction : uint8_t {
    kInput = 0,
    kOutput = 1
  };

  enum State : uint8_t {
    kLow = 0,
    kHigh = 1
  };

  Gpio(uint8_t port, uint8_t pin){
    if (port > 5) {
      sjsu::LogError("Failed to Initialize GPIO driver: "
                     "Invalid GPIO Port %d", port);
      return;
    }

    if (pin > 32) {
      sjsu::LogError("Failed to Initialize GPIO driver: "
                     "Invalid GPIO Pin %d", port);
      return;
    }

    port_ = port;
    pin_ = pin;
    gpio_ = LPC_GPIO[port];

    set_as_output();
    set_low();
  }

  /// Sets direction of GPIO pin
  ///
  /// @param direction - direction to set GPIO pin to
  void set_direction(Direction direction) {
    switch (direction) {
      case Direction::kInput:
        set_as_input();
        break;
      case Direction::kOutput:
        set_as_output();
        break;
      default:
        sjsu::LogError("Invalid GPIO direction: %d", direction);
    }
  }

  /// Sets state of GPIO pin
  ///
  /// @param state - state to set GPIO to
  void set_state(State state) {
    switch (state) {
      case State::kLow:
        set_low();
        break;
      case State::kHigh:
        set_high();
        break;
      default:
        sjsu::LogError("Invalid GPIO state: %d", state);
    }
  }

  /// Toggles state of GPIO pin based on the pin's current state
  void toggle() {
    uint8_t state = get_state();

    switch (state) {
      case State::kLow:
        set_high();
        break;
      case State::kHigh:
        set_low();
        break;
      default:
        sjsu::LogError("Invalid GPIO state: %d", state);
    }
  }

  /// Reads current state of GPIO pin
  ///
  /// @return - current state of GPIO pin
  State get_state() {
    if (gpio_->PIN & (1 << pin_)) {
      return State::kHigh;
    }
    return State::kLow;
  }

  /// Gets defined port
  ///
  /// @return - Returns integer for defined port
  uint8_t get_port() { return port_; };

  /// Gets defined pin
  ///
  /// @return - Returns integer for defined pin
  uint8_t get_pin() { return pin_; };

 private:
  /// Sets GPIO pin direction as input
  void set_as_input() {
    gpio_->DIR &= ~(1 << pin_);
  }

  /// Sets GPIO pin direction as output
  void set_as_output() {
    gpio_->DIR |= (1 << pin_);
  }

  /// Sets GPIO pin state as high
  void set_high() {
    gpio_->PIN |= (1 << pin_);
  }

  /// Sets GPIO pin state as low
  void set_low() {
    gpio_->PIN &= ~(1 << pin_);
  }


  uint8_t pin_;
  uint8_t port_;
  sjsu::lpc40xx::LPC_GPIO_TypeDef* gpio_;

  sjsu::lpc40xx::LPC_GPIO_TypeDef* LPC_GPIO[6] = {
    sjsu::lpc40xx::LPC_GPIO0,
    sjsu::lpc40xx::LPC_GPIO1,
    sjsu::lpc40xx::LPC_GPIO2,
    sjsu::lpc40xx::LPC_GPIO3,
    sjsu::lpc40xx::LPC_GPIO4,
    sjsu::lpc40xx::LPC_GPIO5
  };
};
}  // namespace GpioLab
