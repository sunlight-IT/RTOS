#include "app/event.h"

#include <stdbool.h>
#include <string.h>

#include "log/my_log.h"
#include "tool/memory_detection.h"
#include "zthread.h"

static event_object_t event_table[EVENT_MAX] = {NULL};

// static osMailQId event_queue;

static zThreadOS_t event_thread_os;
static event_type event_priority_table[EVENT_PRIORITY_MAX][10];  // 事件优先级表 10个事件

static void event_dispatch(event_message_t* msg);  // 事件调度

void event_register(event_type type, event_cb cb) {
    if (EVENT_MAX <= type) {
        LOGE("event_register : %d", type);
        return;
    }

    osMutexAcquire(event_thread_os.mutex, 0);
    if (NULL == event_table[type].callback) {
        event_table[type].callback = cb;
    } else {
        LOGE("index: %d is exist", type);
    }
    osMutexRelease(event_thread_os.mutex);
}
void event_send(event_message_t* msg) {
    osMessageQueuePut(event_thread_os.queue, msg, 0, 100);
}
void event_remove(event_type type) {
    if (EVENT_MAX <= type) {
        LOGE("event_register : %d", type);
        return;
    }

    osMutexAcquire(event_thread_os.mutex, 0);
    if (NULL != event_table[type].callback) {
        event_table[type].callback = NULL;
    } else {
        LOGE("index: %d is empty", type);
    }
    osMutexRelease(event_thread_os.mutex);
}

void event_task(void* arg) {
    // osEvent event = osMailGet(event_queue, osWaitForever);

    event_message_t msg;
    uint32_t err;
    err = osMessageQueueGet(event_thread_os.queue, &msg, 0,100);
    LOGI("event_task : %d", err);
    if (osOK == err) {
        event_dispatch(&msg);
    }

    osDelay(100);
}

void event_init(void) {
    // osThreadDef(event_process, event_task, osPriorityAboveNormal, 0, 128);
    // event_thread.id = osThreadCreate(osThread(event_process), NULL);

    // osMutexDef(event_mutex);
    // event_thread.mutex = osMutexCreate(osMutex(event_mutex));

    // osMailQDef(event_queue, 5, event_message_t);
    // event_queue = osMailCreate(osMailQ(event_queue), event_thread.id);

    if (1 != zThread_create(&event_thread_os, "event_process", event_task, osPriorityNormal, sizeof(event_message_t))) {
        LOGE("zThread_create event_process error");
    }
}

void event_schedule(void) {
    if (true == zThread_schedule(&event_thread_os)) {
        LOGI("zThread_schedule event_process");
    } else {
        LOGE("zThread_schedule event_process error");
    }
}

void event_dispatch(event_message_t* msg) {
    if (msg->type < EVENT_NONE || msg->type >= EVENT_MAX) {
        return;
    }
    event_cb cb = NULL;
    if (osOK == osMutexAcquire(event_thread_os.mutex, 0)) {
        if (event_table[msg->type].callback) {
            cb = event_table[msg->type].callback;
        } else {
            LOGE("event_dispatch msg->type: %d is NULL", msg->type);
            osMutexRelease(event_thread_os.mutex);
            return;
        }

        osMutexRelease(event_thread_os.mutex);
        cb(msg->data);
        if (EVENT_STATIC != msg->memory_type) {
            vPortFree(msg);
        }
    }
}

osMessageQueueId_t get_event_msgq(void) { return event_thread_os.queue; }

// void event_priority_process(osEvent event) { event.value.p }

// void event_task_priority(void* arg) {
//     static event_message_t* message;
//     while (1) {
//         osEvent event = osMailGet(event_queue, osWaitForever);
//         message = (event_message_t*)event.value.p;
//         event_priority_table[event_table[message->type].priority]
//     }
// }
