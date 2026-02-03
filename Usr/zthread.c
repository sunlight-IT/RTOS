#include "zthread.h"

#include <stdbool.h>

#include "log/my_log.h"


static void zThread_wait(zThreadOS_t* thread) ;
void thread_process(void* args) {
    if (args == NULL) {
        LOGE("Thread args is NULL!");
        osThreadTerminate(NULL);
    }

    zThreadOS_t* thread = (zThreadOS_t*)args;

    while (1) {
        if (k_Thread_status_waiting == thread->status || k_Thread_status_idle == thread->status) {
            zThread_wait(thread->handle);
            continue;
        }

        while (k_Thread_status_running == thread->status) {
            thread->cb(thread->cb_arg);
        }

        osDelay(1000);
    }
}

uint32_t zThread_create(zThreadOS_t* thread, const char* name, thread_callback cb,
                          osPriority_t priority) {
                            uint32_t err = false;
    if (thread == NULL || cb == NULL) {
        err = false;
        return err;
    }
    thread->cb = cb;
    osMutexAttr_t mutex_attr = {
        .name = "thread_mutex",
        .attr_bits = osMutexPrioInherit,
        .cb_mem = NULL,
        .cb_size = 0,
    };
    thread->mutex = osMutexNew(&mutex_attr);
    osMessageQueueAttr_t queue_attr = {
        .name = "thread_queue",
        .attr_bits = 0,
        .cb_mem = NULL,//用于静态队列内存使用
        .cb_size = 0,
        .mq_mem = NULL,//用于静态队列内存使用
        .mq_size = 0,
    };
    thread->queue = osMessageQueueNew(10, sizeof(uint32_t), &queue_attr);
    
    osThreadAttr_t thread_attr = {
        .name = name,
        .stack_size = 256,
        .priority = priority,
    };
    thread->handle = osThreadNew(thread_process, (void*)thread, &thread_attr);

    thread->status = k_Thread_status_idle;

if (thread->handle == NULL || thread->mutex == NULL || thread->queue == NULL) {
        err = false;
        return err;
    }

    err = true;
    return err;
}

uint8_t zThread_status_set(zThreadOS_t* thread, e_zThread_status status) {
    uint8_t err = false;
    if (osOK != osMutexAcquire(thread->mutex, 100)) {
        return err;
    }
    thread->status = status;
    err = true;
    osMutexRelease(thread->mutex);

    return err;
}
uint8_t zThread_schedule(zThreadOS_t* thread) {
    if (true == zThread_status_set(thread, k_Thread_status_running)) {
        osThreadFlagsSet(thread->handle, 0x01);
        return true;
    }

    return false;
}

void zThread_wait(zThreadOS_t* thread) {
    osThreadFlagsWait(0x01,osFlagsWaitAny, osWaitForever);
}


void zThread_waiting(zThreadOS_t* thread) { zThread_status_set(thread, k_Thread_status_waiting); }
