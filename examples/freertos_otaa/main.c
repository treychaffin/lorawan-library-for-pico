/*
 * FreeRTOS LoRaWAN OTAA Example
 *
 * This example demonstrates how to use the LoRaWAN library with FreeRTOS.
 * It creates separate tasks for LoRaWAN communication and application logic.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "pico/lorawan.h"
#include "tusb.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

// edit with LoRaWAN Node Region and OTAA settings 
#include "config.h"

// Static allocation support functions required by FreeRTOS
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize )
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize )
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

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

// FreeRTOS task handles
static TaskHandle_t xAppTaskHandle = NULL;
static TaskHandle_t xLoRaWANTaskHandle = NULL;
static TimerHandle_t xSendTimer = NULL;

// Application task configuration
#define APP_TASK_STACK_SIZE     1024
#define APP_TASK_PRIORITY       (tskIDLE_PRIORITY + 1)

// LoRaWAN processing task configuration  
#define LORAWAN_TASK_STACK_SIZE 512
#define LORAWAN_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)  // High priority but not max

#define SEND_INTERVAL_MS        60000  // 60 seconds (increased for duty cycle)

// Application task function
static void prvAppTask(void *pvParameters)
{
    (void)pvParameters;
    
    printf("Application task starting...\n");
    printf("Available heap: %d bytes\n", xPortGetFreeHeapSize());
    printf("Task high water mark: %u bytes\n", (unsigned)(uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t)));
    
    printf("FreeRTOS LoRaWAN - Hello OTAA\n\n");
    // Enable LoRaWAN debug for detailed MAC/ACK logs
    lorawan_debug(true);
    
    printf("Initializing LoRaWAN...\n");
    printf("Using Device EUI: %s\n", LORAWAN_DEVICE_EUI);
    printf("Using App EUI: %s\n", LORAWAN_APP_EUI); 
    printf("Using App Key: %s\n", LORAWAN_APP_KEY);
    
    // Initialize LoRaWAN with FreeRTOS support
    if (lorawan_init_otaa(&sx1276_settings, LORAMAC_REGION_US915, &otaa_settings) < 0) {
        printf("LoRaWAN initialization failed!\n");
        vTaskDelete(NULL);
        return;
    }

    printf("LoRaWAN initialized successfully\n");

    // Join the LoRaWAN network
    if (!lorawan_is_joined()) {
        printf("Joining LoRaWAN network...\n");
        printf("Sending join request to network...\n");
        if (lorawan_join_freertos(60000) < 0) {  // 60 second timeout
            printf("Join failed! No response from network\n");
            printf("Check if device is registered in ChirpStack with matching EUIs/Key\n");
            vTaskDelete(NULL);
            return;
        }
        printf("Join successful!\n");
        // Allow MAC to settle and ADR/CFList processing
        printf("Waiting 15 seconds post-join for stabilization...\n");
        vTaskDelay(pdMS_TO_TICKS(15000));
    } else {
        printf("Session contexts restored; skipping OTAA join. DevAddr already assigned.\n");
    }

    // Main application loop
    uint32_t message_count = 0;
    uint32_t confirmed_attempts = 0;
    uint32_t confirmed_acks = 0;
    for (;;) {
        char message[32];
        snprintf(message, sizeof(message), "Confirmed%lu", message_count++);
        
        printf("Sending confirmed message: %s (length: %d)\n", message, (int)strlen(message));
        
        // Send confirmed message on port 1 with 120 second timeout for US915
    printf("Starting confirmed message send...\n");
    // Under FreeRTOS, the library uses semaphores and its internal processing task
    // for precise RX window handling.
    int result = lorawan_send_confirmed_wait(message, strlen(message), 1, 90000);
        confirmed_attempts++;
        int ack = lorawan_last_ack_received();
        
        // Check for any pending downlinks immediately after send
        uint8_t immediate_buffer[242];
        uint8_t immediate_port = 0;
        int immediate_length = lorawan_receive(immediate_buffer, sizeof(immediate_buffer), &immediate_port);
        
        if (immediate_length > -1) {
            printf("IMMEDIATE: Received %d byte downlink on port %d\n", immediate_length, immediate_port);
        } else if (ack) {
            // Suppress noisy message when we already have an ACK
            printf("ACK confirmed (MAC-only downlink, no app payload).\n");
        } else {
            printf("No immediate application payload received (waiting for MAC-only ACK if pending).\n");
        }
        
        if (result == 0) {
            confirmed_acks += ack ? 1 : 0; // Should always be 1 here, but guard for consistency
            printf("Confirmed message sent and acknowledged successfully!\n");
        } else if (result == -2) {
            printf("Confirmed message NOT acknowledged (RX1/RX2 timeout or NACK).\n");
        } else if (result == -3) {
            printf("Send in progress but no TX/confirm event within timeout.\n");
            printf("Check radio readiness, duty cycle or channel availability.\n");
        } else {
            printf("Confirmed message send failed to start (Send rejected). Error: %d\n", result);
        }

        // Diagnostics: DevAddr and ADR state
        uint32_t devaddr = 0; int adr_enabled = 0;
        if (lorawan_get_devaddr(&devaddr) == 0 && lorawan_get_adr_enabled(&adr_enabled) == 0) {
            printf("DevAddr: %08X | ADR: %s\n", devaddr, adr_enabled ? "ON" : "OFF");
        }
        // ACK success ratio
        float ratio = confirmed_attempts ? ((float)confirmed_acks / (float)confirmed_attempts) * 100.0f : 0.0f;
        printf("ACK Ratio: %lu/%lu (%.1f%%)\n", confirmed_acks, confirmed_attempts, ratio);
        
        printf("Available heap: %d bytes\n", xPortGetFreeHeapSize());

        // Check for downlink messages (including ACKs for confirmed messages)
        uint8_t receive_buffer[242];
        uint8_t receive_port = 0;
        int receive_length = lorawan_receive(receive_buffer, sizeof(receive_buffer), &receive_port);
        
        if (receive_length > -1) {
            printf("Received %d byte downlink on port %d: ", receive_length, receive_port);
            for (int i = 0; i < receive_length; i++) {
                printf("%02x", receive_buffer[i]);
            }
            printf("\n");
        }

        // Wait for next send interval  
    printf("Waiting %d seconds before next message...\n", SEND_INTERVAL_MS/1000);
    vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}


// Timer callback for periodic sending (alternative approach)
static void prvSendTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    
    static uint32_t timer_count = 0;
    char message[64];
    snprintf(message, sizeof(message), "Timer message: %lu", timer_count++);
    
    printf("Timer sending: %s\n", message);
    
    // Send unconfirmed message (non-blocking)
    lorawan_send_freertos(message, strlen(message), 2, false, 1000);
}

// FreeRTOS hook functions for debugging
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("STACK OVERFLOW in task: %s\n", pcTaskName);
    for (;;) {
        // Halt execution
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("MALLOC FAILED!\n");
    for (;;) {
        // Halt execution
    }
}

int main(void)
{
    // Initialize stdio and wait for USB CDC connect
    stdio_init_all();

    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }
    
    printf("Starting FreeRTOS LoRaWAN Example...\n");
    printf("FreeRTOS Heap size: %d bytes\n", configTOTAL_HEAP_SIZE);
    printf("Creating application task...\n");

    // Create application task
    if (xTaskCreate(prvAppTask, "AppTask", APP_TASK_STACK_SIZE, NULL, 
                   APP_TASK_PRIORITY, &xAppTaskHandle) != pdPASS) {
        printf("Failed to create application task!\n");
        return -1;
    }
    
    printf("Application task created successfully\n");

    // The library creates its own internal LoRaWAN processing task under FreeRTOS,
    // so we don't create an extra processing task here.

    // Create periodic send timer (optional - demonstrates timer usage)
    // DISABLED: Timer interferes with main task sending
    /*
    xSendTimer = xTimerCreate(
        "SendTimer",
        pdMS_TO_TICKS(60000),  // 1 minute period
        pdTRUE,                // Auto-reload
        NULL,                  // Timer ID
        prvSendTimerCallback   // Callback function
    );
    
    if (xSendTimer != NULL) {
        // Start the timer after 2 minutes (to avoid conflicting with main task)
        xTimerStart(xSendTimer, 0);
    }
    */

    // Start the FreeRTOS scheduler
    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    // Should never reach here
    printf("FreeRTOS scheduler failed to start!\n");
    return -1;
}