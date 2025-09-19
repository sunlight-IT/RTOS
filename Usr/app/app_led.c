#include "app_led.h"

#include "event.h"
#include "gpio.h"
#include "log/my_log.h"
#include "tool/memory_detection.h"

static zThread_t app_message_thread;
static zThread_t app_event_thread;

static void set_callback(zThread_t* thread, thread_callback cb);

static void led_logic(void);
static void led_msg_process(void);
static void led_event_process(void);

// static event_callback event_cb[];

void app_message_task(const void* arg) {
    while (1) {
        // 预留一个任务状态位的处理
        if (app_message_thread.cb) app_message_thread.cb();

        osDelay(10);
    }
}

void app_event_task(const void* arg) {
    while (1) {
        // 预留一个任务状态位的处理

        if (app_event_thread.cb) app_event_thread.cb();

        osDelay(10);
    }
}

void led_event_handler(event_message_t* msg) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    LOGI("BLINK");
}
void led_event_init(void) { event_register(EVENT_LED_BLINK, led_event_handler); }

void app_led_init(void) {
    osThreadDef(led_message, app_message_task, osPriorityAboveNormal, 0, 128);
    app_message_thread.id = osThreadCreate(osThread(led_message), NULL);
    if (!app_message_thread.id) {
        LOGE("%s create error", (osThread(led_message))->name);
    }

    /*msgq init*/
    osMessageQDef(led_message, 10, uint32_t);
    app_message_thread.queue = osMessageCreate(osMessageQ(led_message), NULL);
    /**event group init*/
    if (!app_message_thread.queue) {
        LOGE("%s queue create error", (osThread(led_message))->name);
    }

    app_message_thread.event_group = xEventGroupCreate();
    if (!app_message_thread.event_group) {
        LOGE("%s eventgrup create error", (osThread(led_message))->name);
    }

    osThreadDef(led_event_thread, app_event_task, osPriorityNormal, 0, 128);
    app_event_thread.id = osThreadCreate(osThread(led_event_thread), NULL);

    if (!app_event_thread.id) {
        LOGE("%s create error", (osThread(led_event_thread))->name);
    }

    /*msgq init*/
    osMessageQDef(led_event_message, 10, uint32_t);
    app_event_thread.queue = osMessageCreate(osMessageQ(led_event_message), NULL);

    if (!app_event_thread.queue) {
        LOGE("app_event_thread.queue create error");
    }
    /* event group init*/
    app_event_thread.event_group = xEventGroupCreate();
    if (!app_event_thread.event_group) {
        LOGE("app_event_thread.event_group create error");
    }

    led_event_init();
    check_memory();
}
void app_led_scheduler_start(void) {
    set_callback(&app_message_thread, led_msg_process);
    set_callback(&app_event_thread, led_event_process);
}

void set_callback(zThread_t* thread, thread_callback cb) { thread->cb = cb; }

void led_logic(void) {
    while (1) {
        LOGI("LED TASK");
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);
    }
}

static uint8_t* buf_recv;
void led_msg_process(void) {
    while (1) {
        LOGI("msg TASK");
        app_message_thread.event_message = osMessageGet(app_message_thread.queue, osWaitForever);
        buf_recv = (uint8_t*)app_message_thread.event_message.value.v;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        LOGI("msg: %s\n", (char*)app_message_thread.event_message.value.v);
        LOGI("msg: %04X\n", app_message_thread.event_message.value.v);

        osMessagePut(app_event_thread.queue, app_message_thread.event_message.value.v, 0);

        osDelay(100);
    }
}

void led_event_process(void) {
    while (1) {
        app_event_thread.event_message = osMessageGet(app_event_thread.queue, osWaitForever);
        LOGI("led_event_process TASK");
        osDelay(100);
    }
}

osMessageQId get_led_msgq(void) { return app_message_thread.queue; }
