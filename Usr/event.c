#include "event.h"

#include <stdbool.h>
#include <string.h>

#include "log/my_log.h"
#include "tool/memory_detection.h"
#include "zthread.h"

static event_object_t event_table[EVENT_MAX] = {NULL};

static zThread_t event_priority_thread;
static osMailQId event_queue;

static zThreadOS_t event_thread_os;

static event_type event_priority_table[EVENT_PRIORITY_MAX][10];  // 事件优先级表 10个事件

static void event_dispatch(event_message_t* msg);  // 事件调度

static osal_task_t event_thread_osal;
static osal_queue_t event_thread_queue;
static osal_mutex_t event_thread_mutex;

void event_register(event_type type, event_cb cb) {
    if (EVENT_MAX <= type) {
        LOGE("event_register : %d", type);
        return;
    }

    osal_mutex_acquire(event_thread_mutex, 0);
    if (NULL == event_table[type].callback) {
        event_table[type].callback = cb;
    } else {
        LOGE("index: %d is exist", type);
    }
    osal_mutex_release(event_thread_mutex);
}
void event_remove(event_type type) {
    if (EVENT_MAX <= type) {
        LOGE("event_register : %d", type);
        return;
    }

    osal_mutex_acquire(event_thread_mutex, 0);
    if (NULL != event_table[type].callback) {
        event_table[type].callback = NULL;
    } else {
        LOGE("index: %d is empty", type);
    }
    osal_mutex_release(event_thread_mutex);
}

void event_task(void* arg) {
    // osEvent event = osMailGet(event_queue, osWaitForever);

    event_message_t msg;
    uint32_t err;
    uint32_t notify_value;
    osal_task_notify_wait(0, 0X01, &notify_value, OSAL_WAIT_FOREVER);
    while (true) {
        err = osal_queue_receive(event_thread_queue, (void*)&msg, 100);
        if (OSAL_OK == err) {
            event_dispatch(&msg);
        }
        osal_task_delay(100);
    }
}

void event_init(void) {
    osal_task_config_t config = {
        .name = "event_process",
        .func = (osal_task_func_t)event_task,
        .param = NULL,
        .stack_size = 256 * sizeof(StackType_t),
        .priority = osalPriorityAboveNormal,
    };
    osal_task_create(&config, &event_thread_osal);

    osal_queue_config_t event_queue_config = {
        .name = "event_thread_queue",
        .max_items = 5,
        .item_size = sizeof(event_message_t),
    };
    osal_queue_create(&event_queue_config, &event_thread_queue);

    osal_mutex_config_t mutex_config = {
        .name = "event_thread_mutex",
        .inherit = 1,
    };
    osal_mutex_create(&mutex_config, &event_thread_mutex);

    // if (1 != zThread_create(&event_thread_os, "event_process", event_task, osPriorityNormal)) {
    //     LOGE("zThread_create event_process error");
    // }
}

void event_schedule(void) {
    uint32_t notify_value;
    // if (true == zThread_schedule(&event_thread_os)) {
    //     LOGI("zThread_schedule event_process");
    // } else {
    //     LOGE("zThread_schedule event_process error");
    // }
    osal_task_notify_set(event_thread_osal, 0X01, osal_SetBits, &notify_value);
}

void event_dispatch(event_message_t* msg) {
    if (msg->type < EVENT_NONE || msg->type >= EVENT_MAX) {
        return;
    }
    event_cb cb = NULL;
    uint32_t notify_value;
    if (osOK == osal_mutex_acquire(event_thread_mutex, 0)) {
        if (event_table[msg->type].callback) {
            cb = event_table[msg->type].callback;
        } else {
            LOGE("event_dispatch msg->type: %d is NULL", msg->type);
            osal_mutex_release(event_thread_mutex);
            return;
        }

        osal_mutex_release(event_thread_mutex);
        cb(msg->data);
        if (EVENT_STATIC != msg->memory_type) {
            vPortFree(msg);
        }
    }
}

osal_queue_t get_event_msgq(void) { return event_thread_queue; }

// void event_priority_process(osEvent event) { event.value.p }

// void event_task_priority(void* arg) {
//     static event_message_t* message;
//     while (1) {
//         osEvent event = osMailGet(event_queue, osWaitForever);
//         message = (event_message_t*)event.value.p;
//         event_priority_table[event_table[message->type].priority]
//     }
// }
