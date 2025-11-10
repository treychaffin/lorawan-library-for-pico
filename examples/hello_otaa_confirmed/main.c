/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * This example uses OTAA to join the LoRaWAN network and then sends a
 * "hello world" uplink message periodically and prints out the
 * contents of any downlink message.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/lorawan.h"
#include "tusb.h"

// edit with LoRaWAN Node Region and OTAA settings 
#include "config.h"

// pin configuration for SX1276 radio module - CORRECT Adalogger SPI pins
const struct lorawan_sx1276_settings sx1276_settings = {
    .spi = {
        .inst = spi1,   // Adalogger uses SPI1 for these pins
        .mosi = 15,     // GPIO15 = MOSI on Adalogger
        .miso = 8,      // GPIO8 = MISO on Adalogger
        .sck  = 14,     // GPIO14 = SCK on Adalogger
        .nss  = 10      // Keep NSS the same (any GPIO works)
    },
    .reset = 11,        // Keep reset the same
    .dio0  = 6,         // Keep DIO0 the same
    .dio1  = 5          // Keep DIO1 the same
};

// OTAA settings
const struct lorawan_otaa_settings otaa_settings = {
    .device_eui   = LORAWAN_DEVICE_EUI,
    .app_eui      = LORAWAN_APP_EUI,
    .app_key      = LORAWAN_APP_KEY,
    .channel_mask = LORAWAN_CHANNEL_MASK
};

// variables for receiving data
int receive_length = 0;
uint8_t receive_buffer[242];
uint8_t receive_port = 0;

int main( void )
{
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();

    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }
    
    printf("Pico LoRaWAN - Hello OTAA\n\n");

    // uncomment next line to enable debug
    // lorawan_debug(true);

    // Set confirmed message retry count (1-15, default is usually 8)
    // This means the message will be sent up to 3 times if no ACK is received
    if (lorawan_set_confirmed_retry_count(3) < 0) {
        printf("Failed to set retry count\n");
    } else {
        printf("Set confirmed retry count to 3\n");
    }

    // initialize the LoRaWAN stack
    printf("Initilizating LoRaWAN ... ");
    // printf("[LoRa Debug] sx1276_settings: spi.inst=%p, MOSI=%u, MISO=%u, SCK=%u, NSS=%u, RESET=%u, DIO0=%u, DIO1=%u\n",
    //     sx1276_settings.spi.inst,
    //     sx1276_settings.spi.mosi,
    //     sx1276_settings.spi.miso,
    //     sx1276_settings.spi.sck,
    //     sx1276_settings.spi.nss,
    //     sx1276_settings.reset,
    //     sx1276_settings.dio0,
    //     sx1276_settings.dio1);
    if (lorawan_init_otaa(&sx1276_settings, LORAWAN_REGION, &otaa_settings) < 0) {
        printf("failed!!!\n");
        while (1) {
            tight_loop_contents();
        }
    } else {
        printf("success!\n");
    }

    // Start the join process and wait
    printf("Joining LoRaWAN network ... ");
    lorawan_join();

    while (!lorawan_is_joined()) {
        lorawan_process();
    }
    printf("joined successfully!\n");

    uint32_t last_message_time = 0;

    // loop forever
    while (1) {
        // let the lorwan library process pending events
        lorawan_process();

        // get the current time and see if 5 seconds have passed
        // since the last message was sent
        uint32_t now = to_ms_since_boot(get_absolute_time());

        if ((now - last_message_time) > 5000) {
            const char* message = "hello world!";

            // try to send a confirmed uplink message
            printf("sending confirmed message '%s' ... ", message);
            if (lorawan_send_confirmed(message, strlen(message), 2) < 0) {
                printf("failed!!!\n");
            } else {
                printf("success!\n");
            }

            last_message_time = now;
        }

        // check if a downlink message was received
        receive_length = lorawan_receive(receive_buffer, sizeof(receive_buffer), &receive_port);
        if (receive_length > -1) {
            printf("received a %d byte message on port %d: ", receive_length, receive_port);

            for (int i = 0; i < receive_length; i++) {
                printf("%02x", receive_buffer[i]);
            }
            printf("\n");
        }
    }

    return 0;
}
