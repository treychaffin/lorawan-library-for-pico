/*
 * Advanced LoRaWAN confirmed uplink example with comprehensive features
 * 
 * This example demonstrates:
 * - Confirmed uplinks with retry configuration
 * - Confirmation status checking
 * - ADR control
 * - Data rate and power management
 * - Link check requests
 * - Device time synchronization
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
    
    printf("Pico LoRaWAN - Advanced Confirmed OTAA Example\n\n");

    // uncomment next line to enable debug
    lorawan_debug(true);

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

    // Configure advanced settings
    printf("Configuring advanced settings:\n");
    
    // Set confirmed retry count
    if (lorawan_set_confirmed_retry_count(3) == 0) {
        printf("  ✓ Retry count set to 3\n");
    }
    
    // Enable ADR for automatic optimization
    if (lorawan_set_adr_enabled(false) == 0) {
        printf("  ✓ ADR disabled (prevents auto-transmissions)\n");
    }
    
    // Set initial data rate (DR0-DR5 for US915)
    if (lorawan_set_datarate(0) == 0) {
        printf("  ✓ Data rate set to DR0\n");
    }
    
    // Set transmission power (0 = max power)
    if (lorawan_set_tx_power(0) == 0) {
        printf("  ✓ TX power set to maximum\n");
    }

    // Start the join process and wait
    printf("Joining LoRaWAN network ... ");
    lorawan_join();

    while (!lorawan_is_joined()) {
        lorawan_process();
    }
    printf("joined successfully!\n");

    uint32_t last_message_time = 0;
    uint32_t last_link_check_time = 0;
    uint32_t message_counter = 0;

    // loop forever
    while (1) {
        // let the lorawan library process pending events
        lorawan_process();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Send confirmed message every 120 seconds (2 minutes to allow full stack settling)
        if ((now - last_message_time) > 120000) {
            char message[64];
            sprintf(message, "Hello #%lu DR:%d PWR:%d", 
                   ++message_counter, 
                   lorawan_get_datarate(),
                   lorawan_get_tx_power());

            // Reset confirmation status before attempting to send
            lorawan_reset_confirmed_status();

            // Check if stack is busy and wait if needed
            bool stack_ready = false;
            printf("Sending confirmed message '%s' ... ", message);
            
            // Wait much longer for stack to be ready after acknowledgments
            for (int retry = 0; retry < 200; retry++) { // Wait up to 20 seconds for stack to be ready
                lorawan_process();
                
                if (lorawan_send_confirmed(message, strlen(message), 2) >= 0) {
                    printf("success!\n");
                    stack_ready = true;
                    break;
                }
                
                if (retry == 0) {
                    printf("stack busy, waiting...");
                }
                if (retry % 20 == 0 && retry > 0) { // Show progress every 2 seconds
                    printf(".");
                }
                
                sleep_ms(100);
            }
            
            if (stack_ready) {
                printf("\n");
                
                // Wait for acknowledgment with extended timeout
                uint32_t ack_start = to_ms_since_boot(get_absolute_time());
                bool ack_received = false;
                
                while (to_ms_since_boot(get_absolute_time()) - ack_start < 30000) { // 30 second timeout
                    lorawan_process();
                    
                    if (lorawan_get_last_confirmed_status()) {
                        printf("  ✓ Message was acknowledged by server\n");
                        ack_received = true;
                        break;
                    }
                    
                    sleep_ms(100);
                }
                
                if (!ack_received) {
                    printf("  ✗ Message was NOT acknowledged (timeout)\n");
                } else {
                    // If we got an ACK, give the stack extra time to process automatic responses
                    printf("  → Allowing stack to process acknowledgment responses...\n");
                    
                    // Longer settle time to let all automatic transmissions complete
                    for (int settle = 0; settle < 300; settle++) { // 30 second settle time
                        lorawan_process();
                        sleep_ms(100);
                        
                        // Show progress every 5 seconds
                        if (settle % 50 == 0 && settle > 0) {
                            printf("  → Settling... %d/%d seconds\n", settle/10, 30);
                        }
                    }
                    printf("  → Stack settle period complete\n");
                }
            } else {
                printf(" FAILED! Stack remained busy\n");
            }

            last_message_time = now;
        }

        // Request link check every 300 seconds (5 minutes - much longer interval)
        if ((now - last_link_check_time) > 300000) {
            printf("Requesting link check ... ");
            if (lorawan_request_link_check() == 0) {
                printf("sent!\n");
                
                // Wait longer for response and process more
                for (int i = 0; i < 30; i++) {
                    lorawan_process();
                    sleep_ms(100);
                }
                
                uint8_t demod_margin, nb_gateways;
                if (lorawan_get_link_check_result(&demod_margin, &nb_gateways) == 0) {
                    printf("  Link quality: %d dB margin, %d gateways\n", 
                           demod_margin, nb_gateways);
                } else {
                    printf("  No link check response received\n");
                }
            } else {
                printf("failed!\n");
            }
            
            last_link_check_time = now;
        }

        // check if a downlink message was received
        receive_length = lorawan_receive(receive_buffer, sizeof(receive_buffer), &receive_port);
        if (receive_length > -1) {
            printf("Received %d byte message on port %d: ", receive_length, receive_port);

            for (int i = 0; i < receive_length; i++) {
                printf("%02x", receive_buffer[i]);
            }
            printf("\n");
        }

        sleep_ms(100); // Small delay to prevent busy waiting
    }

    return 0;
}