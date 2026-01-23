#include "core.h"

#include "app/app_led.h"
#include "app/work_queue.h"
#include "app/work_timer.h"
#include "event.h"
#include "gpio.h"
#include "log/my_log.h"
#include "osal_task.h"
#include "tool/debug_light.h"
#include "tool/memory_detection.h"
static const uint8_t s_str_work_queue[] = "I am work queue test message";
static const uint8_t s_str_work_queue2[] = "I am work queue2 test message";
static const uint8_t s_str_work_timer[] = "I am work timer test message";

static work_queue_t work_queue_object;
static work_queue_t work_queue_object_2;
static work_timer_t work_timer;

static osal_task_t defaultTaskHandle;

void StartDefaultTask(void const* argument) { core_main(); }

void osal_init_hook(void) {
    const osal_task_config_t config = {.name = "DefaultTask",
                                       .func = (osal_task_func_t)StartDefaultTask,
                                       .param = NULL,
                                       .stack_size = 128 * sizeof(StackType_t),
                                       .priority = osPriorityNormal,
                                       .time_slice = 0};
    osal_task_create(&config, &defaultTaskHandle);
}
void app_work_queue_init(void) {
    work_queue_init(&work_queue_object);
    work_queue_init(&work_queue_object_2);
}

void app_work_queue_add_single(void (*work_func)(void*), void* arg, TickType_t xValue) {}

void app_work_queue_add_loop(void (*work_func)(void*), void* arg, TickType_t xValue) {}

void app_remove_timer_note(void* arg) { work_timer_node_remove(&work_timer, 1); }
void app_work_timer_init(void) { work_timer_init(&work_timer); }

void app_work_timer_scheduler_start(void) { work_timer_start(&work_timer, 1000); }
void app_work_timer_add(void (*work_func)(void*), void* arg, TickType_t xValue, uint32_t id) {
    work_timer_node_add(&work_timer, id, xValue, work_func, arg);
}

void app_log(void* arg) { LOGI("app_log: %s\n", (char*)arg); }

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
}

event_message_t led_blink = {EVENT_LED_BLINK, NULL, 0};

void core_scheduler(void) {
    QueueHandle_t event_queue = get_event_msgq();
    event_message_t event_message_log = {EVENT_LOG_PRINT, (void*)s_str_work_queue,
                                         sizeof(s_str_work_queue), EVENT_STATIC};
    event_message_t event_message_led = {EVENT_LED_BLINK, NULL, 0, EVENT_STATIC};
    event_schedule();

    while (1) {
        xQueueSendToBack(event_queue, &event_message_led, 100);
        xQueueSendToBack(event_queue, &event_message_log, 100);
        if (xTaskGetTickCount() >= 20000) {
            // app_work_queue_add_single(app_remove_timer_note, NULL, 1);
        }
        osDelay(500);
    }
}

void core_main(void) {
    core_init();
    core_scheduler();
}
