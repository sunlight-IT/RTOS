#include "core.h"

#include "app/work_queue.h"
#include "app/event.h"
#include "gpio.h"
#include "log/my_log.h"
#include "tool/debug_light.h"
#include "tool/memory_detection.h"
static const uint8_t s_str_work_queue[] = "I am work queue test message";
static const uint8_t s_str_work_queue2[] = "I am work queue2 test message";
static const uint8_t s_str_work_timer[] = "I am work timer test message";


void StartDefaultTask(void const* argument) { core_main(); }

void app_work_queue_init(void) {
    work_queue_init();
}

void app_work_queue_add_single(void (*work_func)(void*), void* arg, TickType_t xValue) {}

void app_work_queue_add_loop(void (*work_func)(void*), void* arg, TickType_t xValue) {}


void app_log(char* data) { LOGI("app_log: %s\n", data); }

void app_light(void* arg) {
    DEBUG_LIGHT_TOGGLE;
    LOGI("led loop blink\n");
}

void core_init(void) {
    zlog_init();
    memory_monitor_thread_init();
    event_init();

    event_register(EVENT_LED_BLINK, app_light);
    event_register(EVENT_LOG_PRINT, app_log);

    // app_work_queue_init();
    // app_work_timer_init();
    // memory_monitor_thread_init();

    // app_work_timer_add(app_log, (void *)(s_str_work_timer), 1, 1);

    // work_queue_add(&work_queue_object, 1, app_log, k_WORK_NODE_SIGNAL, (void
    // *)(s_str_work_queue)); work_queue_add(&work_queue_object, 2, app_light, k_WORK_NODE_LOOP,
    // NULL);

    // work_queue_add(&work_queue_object_2, 1, app_log, k_WORK_NODE_SIGNAL,
    //                (void *)(s_str_work_queue2));

    // app_work_queue_add_single(app_remove_timer_note, NULL, 1);
    //  event_init();
    //  check_memory();

    // app_led_init();
    // check_memory();
}

event_message_t led_blink = {EVENT_LED_BLINK, NULL, 0};

void core_scheduler(void) {
    osMessageQueueId_t event_queue = get_event_msgq();
    event_message_t event_message_log = {EVENT_LOG_PRINT, (void*)s_str_work_queue,
                                         sizeof(s_str_work_queue), EVENT_STATIC};
    event_message_t event_message_led = {EVENT_LED_BLINK, NULL, 0, EVENT_STATIC};
    event_schedule();
    // app_led_scheduler_start();
    // app_work_timer_scheduler_start();
    // work_queue_schedule(&work_queue_object);
    // work_queue_schedule(&work_queue_object_2);
int i = 0;
    while (1) {
        // LOGI("SEND EVENT BLINK");
        // LOGI("msg: %s\n", (char *)s_str);
        // LOGI("msg: %04x\n", s_str);
        // osMessagePut(get_led_msgq(), (uint32_t)s_str, 0);
        // osMailPut(get_event_msgq(), &led_blink);
        // DEBUG_LIGHT_TOGGLE;
        event_send(&event_message_led);
        event_send(&event_message_log);
        if (osKernelGetTickCount() >= 20000) {
            // app_work_queue_add_single(app_remove_timer_note, NULL, 1);
        }
        SEGGER_SYSVIEW_Print("PING");
        //  SEGGER_RTT_printf(0, "Hello World!\r\n");
        
        osDelay(500);
    }
}

void core_main(void) {
    core_init();
    core_scheduler();
}
