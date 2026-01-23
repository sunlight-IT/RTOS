#pragma once
#include "cmsis_os.h"

typedef void (*thread_callback)(void*);
typedef void (*event_callback)(void);

typedef struct _zThread_t {
    /* Thread ID */
    osThreadId id;
    const char* name;

    /* Event message queue */
    osMessageQId queue;
    osEvent event_message;

    /* Semaphore */
    osSemaphoreId semaphore;

    /* Mutex */
    osMutexId mutex;

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
    TaskHandle_t handle;
    /* Event message queue */
    QueueHandle_t queue;
    /* Semaphore */
    SemaphoreHandle_t sem;
    /* Mutex */
    SemaphoreHandle_t mutex;
    /* Event group */
    EventGroupHandle_t event_group;

    thread_callback cb;
    void* cb_arg;

    e_zThread_status status;
} zThreadOS_t;

BaseType_t zThread_create(zThreadOS_t* thread, const char* name, thread_callback cb,
                          UBaseType_t priority);
uint8_t zThread_schedule(zThreadOS_t* thread);
void zThread_waiting(zThreadOS_t* thread);
