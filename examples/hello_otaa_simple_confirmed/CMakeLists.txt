cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "adafruit_feather_rp2040_adalogger")

# initialize the Pico SDK
include(../../pico_sdk_import.cmake)

project(pico_lorawan_hello_otaa_simple_confirmed C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    main.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ../../src/include
)

target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_lorawan
    tinyusb_device
    tinyusb_board
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})