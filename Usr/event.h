#pragma once
#include <stdint.h>

#include "cmsis_os.h"

typedef enum {
    EVENT_NONE = 0,
    EVENT_LED_BLINK,
    EVENT_LOG_PRINT,
    EVENT_MAX = 10,
} event_type;

typedef enum {
    EVENT_STATIC,
    EVENT_DYNAMIC,
} event_mesage_memory;

typedef enum {
    EVENT_PRIORITY_LOW = 0,
    EVENT_PRIORITY_MEDIUM,
    EVENT_PRIORITY_HIGH,
    EVENT_PRIORITY_MAX,
} event_priority;

typedef struct {
    event_type type;
    void* data;
    uint8_t data_size;
    event_mesage_memory memory_type;
} event_message_t;

typedef void (*event_cb)(event_message_t* message);

typedef struct {
    event_cb callback;
    uint8_t priority;  // 预留事件优先级
} event_object_t;

void event_init(void);
void event_schedule(void);
// osMailQId get_event_msgq(void);
QueueHandle_t get_event_msgq(void);

void event_register(event_type type, event_cb cb);
