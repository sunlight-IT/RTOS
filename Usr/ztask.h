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

    /* Event group */
    EventGroupHandle_t event_group;
    thread_callback cb;
} zThread_t;
