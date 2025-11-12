#include "freertos-timer-board.h"

// FreeRTOS headers are optional here; the init is a no-op for now.
// If future integration requires RTOS timer primitives, include and wire them.
// #include "FreeRTOS.h"
// #include "timers.h"

void FreeRTOSTimerInit(void) {
    // No-op: LoRaMac uses its own timer.c and the Pico SDK time base.
    // This hook exists so the FreeRTOS build can include a project-specific
    // setup if needed without breaking non-FreeRTOS builds.
}
