#pragma once
#include "pico/stdlib.h"
#include <map>
#include <string>
#include <vector>

namespace my {
class SegmentDisplay {
private:
  const std::vector<unsigned int> m_enable_pins;
  const unsigned int m_srclk_pin;
  const unsigned int m_rclk_pin;
  const unsigned int m_ser_pin;
  bool m_enabled;
  std::string m_text;

  /*
   *   --A--
   *   F   B
   *   --G--
   *   E   C
   *   --D--
   */
  const std::map<char, uint8_t> m_display_lookup{
      //      .GFEDCBA
      {'0', 0b11000000}, {'1', 0b11111001}, {'2', 0b10100100},
      {'3', 0b10110000}, {'4', 0b10011001}, {'5', 0b10010010},
      {'6', 0b10000010}, {'7', 0b11111000}, {'8', 0b10000000},
      {'9', 0b10010000}, {' ', 0b11111111}, {'e', 0b10000110},
      {'r', 0b10101111},
  };

  void set_shift_register(uint8_t value) const {
    printf("Value: %d\n", value);
    gpio_put(this->m_rclk_pin, 0);
    for (size_t index{0}; index < 8; ++index) {
      gpio_put(this->m_srclk_pin, 0);
      gpio_put(this->m_ser_pin, value & (1 << 7 - index));
      gpio_put(this->m_srclk_pin, 1);
    }
    gpio_put(this->m_rclk_pin, 1);
  }

  void set_enable_pins(bool value) const {
    for (size_t index{0}; index < this->m_enable_pins.size(); ++index) {
      gpio_put(this->m_enable_pins[index], value);
    }
  }

public:
  SegmentDisplay(std::vector<unsigned int> enable_pins, unsigned int srclk_pin,
                 unsigned int rclk_pin, unsigned int ser_pin,
                 bool enabled = true)
      : m_enable_pins(enable_pins), m_srclk_pin(srclk_pin),
        m_rclk_pin(rclk_pin), m_ser_pin(ser_pin), m_enabled(enabled) {
    gpio_init(this->m_srclk_pin);
    gpio_set_dir(this->m_srclk_pin, GPIO_OUT);
    gpio_init(this->m_rclk_pin);
    gpio_set_dir(this->m_rclk_pin, GPIO_OUT);
    gpio_init(this->m_ser_pin);
    gpio_set_dir(this->m_ser_pin, GPIO_OUT);

    for (size_t index{0}; index < this->m_enable_pins.size(); ++index) {
      gpio_init(this->m_enable_pins[index]);
      gpio_set_dir(this->m_enable_pins[index], GPIO_OUT);
    }
    this->clear();
  }

  void clear() {
    this->m_text = "";
    this->set_enable_pins(false);
  }

  bool set_text(std::string text) {
    bool error{false};
    if (text.size() > this->m_enable_pins.size()) {
      error = true;
    }
    for (size_t index{0}; index < text.size(); ++index) {
      if (this->m_display_lookup.count(text[index]) < 1) {
        error = true;
        break;
      }
    }
    if (error) {
      this->m_text = this->m_enable_pins.size() >= 3 ? "err" : "e";
    } else {
      this->m_text = text;
    }
    return !error;
  }

  void render() const {
    if (this->m_enabled) {
      printf("Rendering:\n");
      char character{' '};
      for (size_t index{0}; index < this->m_enable_pins.size(); ++index) {
        if (index < this->m_text.size()) {
          character = this->m_text[this->m_text.size() - index - 1];
        } else {
          character = ' ';
        }
        printf("%c\t%x\n", character, this->m_display_lookup.at(character));
        if (index == 0) {
          gpio_put(this->m_enable_pins[this->m_enable_pins.size() - 1], 0);
        } else {
          gpio_put(this->m_enable_pins[index - 1], 0);
        }
        printf("Setting %c %x\n", character,
               this->m_display_lookup.at(character));
        set_shift_register(this->m_display_lookup.at(character));
        gpio_put(this->m_enable_pins[index], 1);
        sleep_ms(1);
      }
    } else {
      this->set_enable_pins(false);
    }
  }

  void set_enabled(bool enabled = true) { this->m_enabled = enabled; }
};
} // namespace my
