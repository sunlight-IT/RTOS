#include "debug_light.h"

#include "log/my_log.h"
#include "tool/tick.h"
#define TAG "debug_light"

static uint32_t last_time;

void debug_light(void) {
    uint32_t pass_time;

    pass_time = has_pass_time(last_time);

    if (pass_time > DEBUG_LIGHT_BLINK) {
        DEBUG_LIGHT_TOGGLE;
        last_time = HAL_GetTick();
    }
}