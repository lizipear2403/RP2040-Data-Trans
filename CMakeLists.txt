cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to a Pico SDK 2.x checkout.")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(rp2040_data_trans C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(rp2040_data_trans main.c nrf24.c)
target_link_libraries(rp2040_data_trans PRIVATE pico_stdlib hardware_gpio hardware_spi)
pico_enable_stdio_usb(rp2040_data_trans 1)
pico_enable_stdio_uart(rp2040_data_trans 0)
pico_add_extra_outputs(rp2040_data_trans)
