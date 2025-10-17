#include "zthread.h"

void thread_process(void* args) {
    zThreadOS_t* thread = (zThreadOS_t*)args;

    thread->cb();
}

zThreadOS_t* zThread_create(zThreadOS_t* thread, const* name, thread_callback cb) {
    thread->cb = cb;
    thread->mutex = xSemaphoreCreateMutex();
    thread->queue = xQueueCreate(10, sizeof(osEvent));

    xTaskCreate(thread_process, name, 128, (void*)thread, 5, &(thread->handle));
}