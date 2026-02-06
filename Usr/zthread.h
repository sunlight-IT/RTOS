#pragma once
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"


typedef void (*thread_callback)(void*);
typedef void (*event_callback)(void);

typedef struct _zThread_t {
    /* Thread ID */
    TaskHandle_t id;
    const char* name;

    /* Event message queue */
    QueueHandle_t    queue;
    EventBits_t event_message;

    /* Semaphore */
    SemaphoreHandle_t semaphore;

    /* Mutex */
    SemaphoreHandle_t mutex;

    /* Event group */
    EventGroupHandle_t event_group;
    thread_callback cb;

} zThread_t;

typedef enum zThread_status {
    k_Thread_status_idle = 0,
    k_Thread_status_running,
    k_Thread_status_waiting,
    k_Thread_status_suspended,
    k_Thread_status_deleted,
    k_Thread_status_error,
    k_Thread_status_max
} e_zThread_status;

typedef struct _zThreadOS_t {
    /* Thread ID */
    osThreadId_t handle;
    /* Event message queue */
    osMessageQueueId_t queue;
    /* Semaphore */
    osSemaphoreId_t semaphore;
    /* Mutex */
    osMutexId_t mutex;
    /* Event group */
    // osEventFlagsId_t event_group;

    thread_callback cb;
    void* cb_arg;

    e_zThread_status status;
} zThreadOS_t;

uint32_t zThread_create(zThreadOS_t* thread, const char* name, thread_callback cb,
                          osPriority_t priority,size_t msg_size);
uint8_t zThread_schedule(zThreadOS_t* thread);
void zThread_waiting(zThreadOS_t* thread);
