/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * This example uses OTAA to join the LoRaWAN network and then sends
 * confirmed uplink messages periodically.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/lorawan.h"
#include "tusb.h"

// edit with LoRaWAN Node Region and OTAA settings 
#include "config.h"

// pin configuration for SX1276 radio module
const struct lorawan_sx1276_settings sx1276_settings = {
    .spi = {
        .inst = PICO_DEFAULT_SPI_INSTANCE(),
        .mosi = PICO_DEFAULT_SPI_TX_PIN,
        .miso = PICO_DEFAULT_SPI_RX_PIN,
        .sck  = PICO_DEFAULT_SPI_SCK_PIN,
        .nss  = 10
    },
    .reset = 11,
    .dio0  = 6,
    .dio1  = 5
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

    printf("Pico LoRaWAN - Simple Confirmed OTAA Example\n\n");

    // uncomment next line to enable debug
    // lorawan_debug(true);

    // initialize the LoRaWAN stack
    printf("Initializing LoRaWAN ... ");
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
    uint32_t message_counter = 0;

    // loop forever
    while (1) {
        // let the lorawan library process pending events
        lorawan_process();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Send confirmed message every 60 seconds
        if ((now - last_message_time) > 60000) {
            char message[64];
            sprintf(message, "Hello confirmed #%lu", ++message_counter);

            printf("Sending confirmed message: '%s' ... ", message);
            
            // Send confirmed message
            if (lorawan_send_confirmed(message, strlen(message), 2) >= 0) {
                printf("sent successfully!\n");
                
                // Give some time for acknowledgment processing
                uint32_t wait_start = to_ms_since_boot(get_absolute_time());
                while (to_ms_since_boot(get_absolute_time()) - wait_start < 15000) {
                    lorawan_process();
                    
                    // Check for acknowledgment
                    if (lorawan_get_last_confirmed_status()) {
                        printf("  ✓ Message was acknowledged by server\n");
                        break;
                    }
                    
                    sleep_ms(100);
                }
                
                if (!lorawan_get_last_confirmed_status()) {
                    printf("  ? No acknowledgment received\n");
                }
                
            } else {
                printf("failed to send!\n");
            }

            last_message_time = now;
        }

        // check for downlink messages
        receive_length = lorawan_receive(receive_buffer, sizeof(receive_buffer), &receive_port);
        if (receive_length > -1) {
            printf("received a %d byte message on port %d: ", receive_length, receive_port);

            for (int i = 0; i < receive_length; i++) {
                printf("%02x", receive_buffer[i]);
            }
            printf("\n");
        }

        sleep_ms(100);
    }

    return 0;
}