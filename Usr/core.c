#include "core.h"

#include "app/app_led.h"
#include "cmsis_os.h"
#include "event.h"
#include "gpio.h"
#include "log/my_log.h"

static uint8_t s_str[] = "I am core task";
void StartDefaultTask(void const *argument) { core_main(); }

void core_init(void) {
    zlog_init();

    event_init();
    app_led_init();
}

event_message_t led_blink = {EVENT_LED_BLINK, NULL, 0};
void core_scheduler(void) {
    app_led_scheduler_start();

    while (1) {
        LOGI("SEND EVENT BLINK");
        LOGI("msg: %s\n", (char *)s_str);
        LOGI("msg: %04x\n", s_str);
        osMessagePut(get_led_msgq(), (uint32_t)s_str, 0);
        osMailPut(get_event_msgq(), &led_blink);
        osDelay(500);
    }
}

void core_main(void) {
    core_init();
    core_scheduler();
}
