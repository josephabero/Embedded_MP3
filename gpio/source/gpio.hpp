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

  enum Level : uint8_t {
    kLow = 0,
    kHigh = 1
  };

  Gpio(uint8_t port, uint8_t pin){
    if (port > 5) {
      sjsu::LogError("Failed to Initialize GPIO driver: Invalid GPIO Port %d", port);
      return;
    }

    if (pin > 32) {
      sjsu::LogError("Failed to Initialize GPIO driver: Invalid GPIO Pin %d", port);
      return;
    }

    port_ = port;
    pin_ = pin;
    gpio_ = LPC_GPIO[port];

    set_as_output();
    set_low();
  }

  void set_as_input(){
    gpio_->DIR &= ~(1 << pin_);
  }

  void set_as_output(){
    gpio_->DIR |= (1 << pin_);
  }

  void set_high(){
    gpio_->PIN |= (1 << pin_);
  }

  void set_low(){
    gpio_->PIN &= ~(1 << pin_);
  }

  void set(bool state){
    if (state) {
      set_high();
    }
    else {
      set_low();
    }
  }

  void toggle() {
    if (get_level() == kLow) {
      set_high();
    }
    else {
      set_low();
    }
  }

  Level get_level(){
    if (gpio_->PIN & (1 << pin_)) {
      return kHigh;
    }
    return kLow;
  }

  uint8_t get_pin(){ return pin_; };
  
  uint8_t get_port(){ return port_; };

 private:
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
}