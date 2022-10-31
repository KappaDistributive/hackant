cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

project(hackant)

add_executable(main src/main.cpp)
target_link_libraries(main pico_stdlib)
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
pico_add_extra_outputs(main)

add_executable(display_demo src/display_demo.cpp)
target_link_libraries(display_demo pico_stdlib pico_multicore)
pico_enable_stdio_usb(display_demo 1)
pico_enable_stdio_uart(display_demo 0)
pico_add_extra_outputs(display_demo)
