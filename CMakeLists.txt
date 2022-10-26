cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

project(pico-test)

add_executable(main src/main.cpp)
target_link_libraries(main pico_stdlib)
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
pico_add_extra_outputs(main)
