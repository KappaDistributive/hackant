// Copyright 2022 Stefan Mesken
#include <stdio.h>
#include <string>

#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#define UART_ID uart1
#define BAUD_RATE 19230 // ~ bit timing of 52us
#define DATA_BITS 8
#define STOP_BITS 1
#define LIN_SYNC_MASK 0x55
#define LIN_POSITION_PID 0x92
#define PARITY UART_PARITY_NONE
#define UART_RX_PIN 5

#define POSITION_MIN 170
#define POSITION_MAX 6400
#define POSITION_SIT 1800
#define POSITION_STAND 5700
#define POSITION_MARGIN 120
#define POSITION_TO_HEIGHT_SLOPE 0.0973
#define POSITION_TO_HEIGHT_BIAS 628.73

#define SHORT_PRESS_DURATION 250
#define DEBOUNCE_DURATION 50
#define TICK_DURATION 200
#define TX_UP 6
#define BUTTON_UP 7
#define TX_DOWN 8
#define BUTTON_DOWN 9

class Table {
 private:
  uint16_t m_position;
  bool m_button[2];
  uint32_t m_button_ts[2];
  uint32_t m_button_duration[2];
  bool m_target[2];

  void set_button(size_t index, uint32_t events) {
    if (index > 1)
      return;
    auto ts = to_ms_since_boot(get_absolute_time());
    auto duration = ts - this->m_button_ts[index];
    if (duration < DEBOUNCE_DURATION) {
      printf("Debouncing button press after %dms\n", duration);
      return;
    }
    bool value{false};
    switch (events) {
    case 4:
      value = true;
      break;
    case 12:
      value = !this->m_button[index];
      break;
    default:
      break;
    }
    this->m_button[index] = value;
    this->m_button_duration[index] = ts - this->m_button_ts[index];
    this->m_button_ts[index] = ts;

    if (!this->m_button[index] &&
        this->m_button_duration[index] < SHORT_PRESS_DURATION) {
      if ((index == 0 && this->m_position < POSITION_STAND) ||
          (index == 1 && this->m_position > POSITION_SIT)) {
        this->m_target[index] = true;
      }
    }
  }

  bool position_is_valid(uint16_t position) const {
    return (position >= POSITION_MIN && position <= POSITION_MAX);
  }

  float position_to_height(uint16_t position) const {
    // Converts table position as reported on the LIN bus to height in
    // millimeters.
    if (position_is_valid(position)) {
      return POSITION_TO_HEIGHT_SLOPE * static_cast<float>(position) + POSITION_TO_HEIGHT_BIAS;
    }
    return -1.0;
  }

 public:
  Table()
      : m_position(0), m_button{false, false}, m_button_ts{0, 0},
        m_button_duration{0, 0}, m_target{false, false} {}

  void set_button_up(uint32_t events) { this->set_button(0, events); }

  void set_button_down(uint32_t events) { this->set_button(1, events); }

  void set_position(uint16_t position) {
    if (this->position_is_valid(position)) {
      this->m_position = position;
    } else {
      printf("Attempted to set table position to an invalid value: %d\n",
             position);
    }
  }

  float height() const { return this->position_to_height(this->m_position); }

  uint16_t position() const { return this->m_position; }

  bool move_down() {
    bool move{false};
    if (this->m_button[1]) {
      this->m_target[0] = false;
      this->m_target[1] = false;
      move = true;
    } else if (this->m_target[1]) {
      if (static_cast<int>(this->position()) > POSITION_SIT + POSITION_MARGIN) {
        move = true;
      } else {
        printf("Reached POSITION_SIT: %d\t%.0f\n", this->position(),
               this->height());
        this->m_target[0] = false;
        this->m_target[1] = false;
        move = false;
      }
    }
    return move;
  }

  bool move_up() {
    bool move{false};
    if (this->m_button[0]) {
      this->m_target[0] = false;
      this->m_target[1] = false;
      move = true;
    } else if (this->m_target[0]) {
      if (static_cast<int>(this->position()) + POSITION_MARGIN < POSITION_STAND) {
        move = true;
      } else {
        printf("Reached POSITION_STAND: %d\t%.0f\n", this->position(),
               this->height());
        this->m_target[0] = false;
        this->m_target[1] = false;
        move = false;
      }
    }
    return move;
  }
};

static Table table;
static uint32_t tick_ts{0};

void on_uart_rx() {
  uint8_t buffer[16];
  uart_read_blocking(UART_ID, buffer, 16);
  for (size_t index = 0; index < 11; index++) {
    if (buffer[index] == LIN_SYNC_MASK && buffer[index + 1] == LIN_POSITION_PID) {
      // The table position is a two byte value. LSB is sent first.
      uint16_t position{buffer[index + 3]};
      position <<= 8;
      position |= buffer[index + 2];
      table.set_position(position);
    }
  }
}

void button_callback(unsigned int gpio, uint32_t events) {
  printf("Button %d fired events %d\n", gpio, events);
  if (gpio == static_cast<unsigned int>(BUTTON_UP)) {
    table.set_button_up(events);
  } else if (gpio == static_cast<unsigned int>(BUTTON_DOWN)) {
    table.set_button_down(events);
  }
}

int main() {
  stdio_init_all();

  // set up UART
  int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
  uart_init(UART_ID, BAUD_RATE);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);
  uart_set_hw_flow(UART_ID, false, false);
  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
  irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
  irq_set_enabled(UART_IRQ, true);
  uart_set_irq_enables(UART_ID, true, false);

  // set up GPIO
  gpio_init(TX_UP);
  gpio_set_dir(TX_UP, GPIO_OUT);
  gpio_init(BUTTON_UP);
  gpio_set_dir(BUTTON_UP, GPIO_IN);
  gpio_pull_up(BUTTON_UP);
  gpio_set_irq_enabled_with_callback(BUTTON_UP,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, &button_callback);
  gpio_init(TX_DOWN);
  gpio_set_dir(TX_DOWN, GPIO_OUT);
  gpio_init(BUTTON_DOWN);
  gpio_set_dir(BUTTON_DOWN, GPIO_IN);
  gpio_pull_up(BUTTON_DOWN);
  gpio_set_irq_enabled_with_callback(BUTTON_DOWN,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, &button_callback);

  while (1) {
    auto ts = to_ms_since_boot(get_absolute_time());
    if (ts > tick_ts + TICK_DURATION) {
      tick_ts = ts;
      printf("Position: %d\t%.0fmm\n", table.position(), table.height());
    }
    if (table.move_down()) {
      gpio_put(TX_DOWN, 1);
    } else if (table.move_up()) {
      gpio_put(TX_UP, 1);
    } else {
      gpio_put(TX_DOWN, 0);
      gpio_put(TX_UP, 0);
    }
  }
  return 0;
}
