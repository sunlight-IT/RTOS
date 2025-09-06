#include "core.h"

#include "app/app_led.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "log/my_log.h"

static uint8_t s_str[] = "I am core task";
void StartDefaultTask(void const *argument) { core_main(); }

void core_init(void) {
    zlog_init();
    app_led_init();
}

void core_scheduler(void) {
    app_led_scheduler_start();

    while (1) {
        LOGI("CORE TASK");
        LOGI("MSG SEND IS: %s\n", s_str);
        LOGI("MSG SEND ADDR: %04X\n", s_str);
        osMessagePut(get_led_msgq(), (uint32_t)s_str, 100);

        osDelay(500);
    }
}

void core_main(void) {
    core_init();
    core_scheduler();
}
