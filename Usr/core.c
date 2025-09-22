#include "core.h"

#include "app/app_led.h"
#include "app/work_queue.h"
#include "event.h"
#include "gpio.h"
#include "log/my_log.h"
#include "tool/debug_light.h"
static const uint8_t s_str[] = "I am work queue test message";

static work_queue_t work_queue;

void app_work_queue_init(void) { work_queue_init(&work_queue); }

void app_work_queue_add(void (*work_func)(void *), void *arg, TickType_t xValue) {
    work_queue_add(&work_queue, xValue, work_func, arg);
}
void StartDefaultTask(void const *argument) { core_main(); }

void core_init(void) {
    zlog_init();
    memory_monitor_thread_init();

    app_work_queue_init();

    // event_init();
    // check_memory();

    // app_led_init();
    // check_memory();
}

event_message_t led_blink = {EVENT_LED_BLINK, NULL, 0};

void app_log(void *arg) { LOGI("app_log: %s\n", (char *)arg); }
void app_light(void *arg) { DEBUG_LIGHT_TOGGLE; }
void core_scheduler(void) {
    // app_led_scheduler_start();

    while (1) {
        // LOGI("SEND EVENT BLINK");
        // LOGI("msg: %s\n", (char *)s_str);
        // LOGI("msg: %04x\n", s_str);
        // osMessagePut(get_led_msgq(), (uint32_t)s_str, 0);
        // osMailPut(get_event_msgq(), &led_blink);
        // DEBUG_LIGHT_TOGGLE;
        app_work_queue_add(app_log, (void *)(s_str), 1);
        app_work_queue_add(app_light, NULL, 2);
        osDelay(500);
    }
}

void core_main(void) {
    core_init();
    core_scheduler();
}
