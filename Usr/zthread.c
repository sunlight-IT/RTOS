#include "zthread.h"

#include <stdbool.h>

#include "log/my_log.h"

void thread_process(void* args) {
    if (args == NULL) {
        LOGE("Thread args is NULL!");
        osThreadTerminate(NULL);
    }

    zThreadOS_t* thread = (zThreadOS_t*)args;

    while (1) {
        if (k_Thread_status_waiting == thread->status || k_Thread_status_idle == thread->status) {
            osSignalWait(0x01, osWaitForever);
            continue;
        }

        while (k_Thread_status_running == thread->status) {
            thread->cb(thread->cb_arg);
        }

        osDelay(1000);
    }
}

BaseType_t zThread_create(zThreadOS_t* thread, const char* name, thread_callback cb,
                          UBaseType_t priority) {
    thread->cb = cb;
    thread->mutex = xSemaphoreCreateMutex();
    thread->queue = xQueueCreate(10, sizeof(osEvent));
    thread->status = k_Thread_status_idle;
    return xTaskCreate(thread_process, name, 256, (void*)thread, priority, &(thread->handle));
}

uint8_t zThread_status_set(zThreadOS_t* thread, e_zThread_status status) {
    uint8_t err = false;
    if (osOK != osMutexWait(thread->mutex, 100)) {
        return err;
    }
    thread->status = status;
    err = true;
    osMutexRelease(thread->mutex);

    return err;
}
uint8_t zThread_schedule(zThreadOS_t* thread) {
    if (true == zThread_status_set(thread, k_Thread_status_running)) {
        osSignalSet(thread->handle, 0x01);
        return true;
    }

    return false;
}

void zThread_waiting(zThreadOS_t* thread) { zThread_status_set(thread, k_Thread_status_waiting); }
