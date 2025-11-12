add_executable(pico_lorawan_freertos_otaa
    main.c
)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico_lorawan_freertos_otaa)

target_link_libraries(pico_lorawan_freertos_otaa
    pico_stdlib
    pico_lorawan
    freertos_kernel
)
# FreeRTOS requires static allocation hook implementations; provided in main.c.
target_compile_definitions(pico_lorawan_freertos_otaa PRIVATE USE_FREERTOS=1)


pico_enable_stdio_usb(pico_lorawan_freertos_otaa 1)
pico_enable_stdio_uart(pico_lorawan_freertos_otaa 0)

# Use top-level PICO_BOARD; override here only if you intentionally target another board
# set(PICO_BOARD adafruit_feather_rp2040_adalogger)