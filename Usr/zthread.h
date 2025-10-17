#pragma once
#include "cmsis_os.h"

typedef void (*thread_callback)(void);
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
} zThreadOS_t;
