#ifndef FREERTOS_TIMER_BOARD_H
#define FREERTOS_TIMER_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

// Initializes any board-specific timer glue when running under FreeRTOS.
// For this project, the LoRaMac timer implementation is independent,
// so this can be a no-op. Provided to satisfy includes when USE_FREERTOS=1.
void FreeRTOSTimerInit(void);

#ifdef __cplusplus
}
#endif

#endif // FREERTOS_TIMER_BOARD_H
