#include "core.h"

#include "cmsis_os.h"
#include "gpio.h"

static osThreadId defaultTaskHandle;

__weak void StartDefaultTask(void const *argument) {
    for (;;) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);
    }
}

void core_init(void) {
    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    // osKernelStart();
}

void core_loop(void) {
    // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    // HAL_Delay(500);
}

void core_main(void) {
    core_init();
    while (1) {
        core_loop();
    }
}
