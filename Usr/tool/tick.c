#include "tick.h"

#include "cmsis_os.h"
#include "core.h"

uint32_t has_pass_time(uint32_t last_time) {
    int32_t pass_time;

#if _RTOS
    pass_time = osKernelSysTick() - last_time;
#else
    pass_time = HAL_GetTick() - last_time;
#endif

    if (pass_time >= 0)
        return pass_time;
    else
        return UINT32_MAX - last_time - pass_time;
}