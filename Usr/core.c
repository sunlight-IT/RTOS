#include "core.h"

#include "app/app_led.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "log/my_log.h"
#include "ztask.h"

void StartDefaultTask(void const *argument) { core_main(); }

void core_init(void) { app_led_init(); }

void core_loop(void) {
    while (1) {
        LOGI("core_loop");
        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        osDelay(100);
    }
}

void core_main(void) {
    core_init();
    core_loop();
}
