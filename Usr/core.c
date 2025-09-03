#include "core.h"

#include "cmsis_os.h"
#include "gpio.h"
#include "log/my_log.h"

void StartDefaultTask(void const *argument) { core_main(); }

void core_init(void) {}

void core_loop(void) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    osDelay(500);
}

void core_main(void) {
    core_init();
    while (1) {
        core_loop();
    }
}
