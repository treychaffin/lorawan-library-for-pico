# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pico_lorawan)

# initialize the Pico SDK
pico_sdk_init()

# FreeRTOS configuration (default OFF unless explicitly enabled)
option(USE_FREERTOS "Enable FreeRTOS support" OFF)

set(LORAMAC_NODE_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/LoRaMac-node)

add_library(pico_loramac_node INTERFACE)

target_sources(pico_loramac_node INTERFACE
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/CayenneLpp.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/cli.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandlerMsgDisplay.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/NvmDataMgmt.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/LmHandler.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/packages/FragDecoder.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/packages/LmhpClockSync.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/packages/LmhpCompliance.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/packages/LmhpFragmentation.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/packages/LmhpRemoteMcastSetup.c
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/NvmDataMgmt.c

    ${LORAMAC_NODE_PATH}/src/boards/mcu/utilities.c

    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/NvmDataMgmt.c

    ${LORAMAC_NODE_PATH}/src/mac/region/Region.c
    ${LORAMAC_NODE_PATH}/src/mac/region/RegionCommon.c
    ${LORAMAC_NODE_PATH}/src/mac/region/RegionBaseUS.c
    ${LORAMAC_NODE_PATH}/src/mac/region/RegionUS915.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMac.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacAdr.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacClassB.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacCommands.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacConfirmQueue.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacCrypto.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacParser.c
    ${LORAMAC_NODE_PATH}/src/mac/LoRaMacSerializer.c

    ${LORAMAC_NODE_PATH}/src/peripherals/soft-se/aes.c
    ${LORAMAC_NODE_PATH}/src/peripherals/soft-se/cmac.c
    ${LORAMAC_NODE_PATH}/src/peripherals/soft-se/soft-se-hal.c
    ${LORAMAC_NODE_PATH}/src/peripherals/soft-se/soft-se.c

    ${LORAMAC_NODE_PATH}/src/radio/sx1276/sx1276.c

    ${LORAMAC_NODE_PATH}/src/system/delay.c
    ${LORAMAC_NODE_PATH}/src/system/gpio.c
    ${LORAMAC_NODE_PATH}/src/system/nvmm.c
    ${LORAMAC_NODE_PATH}/src/system/systime.c
    ${LORAMAC_NODE_PATH}/src/system/timer.c

    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/delay-board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/eeprom-board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/gpio-board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/rtc-board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/spi-board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/boards/rp2040/sx1276-board.c
)

# FreeRTOS sources will be added by the specific FreeRTOS examples, not globally

target_include_directories(pico_loramac_node INTERFACE
    ${LORAMAC_NODE_PATH}/src
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler
    ${LORAMAC_NODE_PATH}/src/apps/LoRaMac/common/LmHandler/packages
    ${LORAMAC_NODE_PATH}/src/boards
    ${LORAMAC_NODE_PATH}/src/mac
    ${LORAMAC_NODE_PATH}/src/mac/region
    ${LORAMAC_NODE_PATH}/src/peripherals/soft-se
    ${LORAMAC_NODE_PATH}/src/radio
    ${LORAMAC_NODE_PATH}/src/system
)

target_link_libraries(pico_loramac_node INTERFACE pico_stdlib pico_unique_id hardware_spi)

# Add FreeRTOS support (kernel) to the build if enabled, but do not link globally
if(USE_FREERTOS)
    include(FetchContent)
    FetchContent_Declare(
        freertos_kernel
        GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
        GIT_TAG        V10.6.1
    )
    # Provide project config directory for FreeRTOS
    set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/src/freertos" CACHE STRING "")
    set(FREERTOS_PORT "GCC_ARM_CM0" CACHE STRING "")
    FetchContent_MakeAvailable(freertos_kernel)
endif()

target_compile_definitions(pico_loramac_node INTERFACE -DSOFT_SE)
# Only build US915 region
target_compile_definitions(pico_loramac_node INTERFACE -DREGION_US915)
target_compile_definitions(pico_loramac_node INTERFACE -DACTIVE_REGION=LORAMAC_REGION_US915)

add_library(pico_lorawan INTERFACE)

target_sources(pico_lorawan INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/lorawan.c
)

target_include_directories(pico_lorawan INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)

target_link_libraries(pico_lorawan INTERFACE pico_loramac_node)

# If FreeRTOS is enabled, add the board timer shim source and its include path.
if(USE_FREERTOS)
    target_sources(pico_lorawan INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/freertos/freertos-timer-board.c
    )
    target_include_directories(pico_lorawan INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/freertos
    )
endif()

add_subdirectory("examples/default_dev_eui")
add_subdirectory("examples/erase_nvm")
add_subdirectory("examples/hello_abp")
add_subdirectory("examples/hello_otaa")
add_subdirectory("examples/hello_otaa_confirmed")
add_subdirectory("examples/otaa_temperature_led")

# Add FreeRTOS example if FreeRTOS is enabled
if(USE_FREERTOS)
    add_subdirectory("examples/freertos_otaa")
endif()
