cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to a Pico SDK 2.x checkout.")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(rp2040_data_trans C CXX ASM)
pico_sdk_init()

include(FetchContent)
FetchContent_Declare(rf24
    GIT_REPOSITORY https://github.com/nRF24/RF24.git
    GIT_TAG b11ffc3226ba886f86ac1ca0cd9dc25bb94fd496 # v1.6.2
)
FetchContent_MakeAvailable(rf24)

add_executable(rp2040_data_trans main.cpp)
target_compile_features(rp2040_data_trans PRIVATE cxx_std_17)
target_link_libraries(rp2040_data_trans PRIVATE pico_stdlib hardware_gpio hardware_spi RF24)
pico_enable_stdio_usb(rp2040_data_trans 1)
pico_enable_stdio_uart(rp2040_data_trans 0)
pico_add_extra_outputs(rp2040_data_trans)
