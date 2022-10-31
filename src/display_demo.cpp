// Copyright 2022 Stefan Mesken
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "segment_display.hpp"

#define DISPLAY_ENABLE_0 18
#define DISPLAY_ENABLE_1 19
#define DISPLAY_ENABLE_2 20
#define DISPLAY_ENABLE_3 21
#define DISPLAY_SRCLK 11
#define DISPLAY_RCLK 12
#define DISPLAY_SER 13

int main() {
  stdio_init_all();
  static my::SegmentDisplay display(
      {DISPLAY_ENABLE_0, DISPLAY_ENABLE_1, DISPLAY_ENABLE_2, DISPLAY_ENABLE_3},
      DISPLAY_SRCLK, DISPLAY_RCLK, DISPLAY_SER);

  unsigned int x = 0;

  multicore_launch_core1([]() {
    while (true)
      display.render();
  });

  while (true) {
    printf("Waiting...\n");
    display.set_text(std::to_string(x));
    x = (x + 1) % 10000;
    sleep_ms(1000);
  }

  return 0;
}
