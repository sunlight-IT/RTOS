#include "event.h"

#include <string.h>

#include "log/my_log.h"
#include "tool/memory_detection.h"
#include "zthread.h"

static event_object_t event_table[EVENT_MAX] = {NULL};

static zThread_t event_thread;
static zThread_t event_priority_thread;
static osMailQId event_queue;

static event_type event_priority_table[EVENT_PRIORITY_MAX][10];  // 事件优先级表 10个事件

static void event_dispatch(event_type type, event_message_t* data);  // 事件调度

void event_register(event_type type, event_cb cb) { event_table[type].callback = cb; }

void event_task(const void* arg) {
    while (1) {
        osEvent event = osMailGet(event_queue, osWaitForever);
        LOGI("event_task : %d", event.status);
        if (event.status == osEventMail) {
            event_message_t* msg = event.value.p;
            event_dispatch(msg->type, msg->data);
        }
        osDelay(10);
    }
}

void event_init(void) {
    osThreadDef(event_process, event_task, osPriorityAboveNormal, 0, 128);
    event_thread.id = osThreadCreate(osThread(event_process), NULL);

    // osThreadDef(event_priority, event_task_priority, osPriorityAboveNormal, 0, 128);
    // event_priority_thread.id = osThreadCreate(osThread(event_priority), NULL);

    osMailQDef(event_queue, 5, event_message_t);
    event_queue = osMailCreate(osMailQ(event_queue), event_thread.id);

    check_memory();
}
void event_dispatch(event_type type, event_message_t* data) {
    if (type < EVENT_NONE || type >= EVENT_MAX) {
        return;
    }
    if (event_table[type].callback) {
        event_table[type].callback(data);
    }
}

osMailQId get_event_msgq(void) { return event_queue; }

// void event_priority_process(osEvent event) { event.value.p }

// void event_task_priority(void* arg) {
//     static event_message_t* message;
//     while (1) {
//         osEvent event = osMailGet(event_queue, osWaitForever);
//         message = (event_message_t*)event.value.p;
//         event_priority_table[event_table[message->type].priority]
//     }
// }
