#include "memory_detection.h"

#include "component.h"
#include "log/my_log.h"

static zThread_t memory_thread;
static zThreadOS_t memory_thread_os;
static osal_task_t memory_thread_osal;
static osal_mutex_t memory_mutex;
static osal_queue_t memory_queue;
void memory_monitor_thread(void) {
    size_t xFreeHeapSize, minEverFreeHeapSize;
    uint32_t notify_value;
    LOGI("memory_monitor_thread start");

    while (1) {
        osal_task_notify_wait(0, 0X01, &notify_value, OSAL_WAIT_FOREVER);
        xFreeHeapSize = osal_port_freertos_memory_get_free_size();
        minEverFreeHeapSize = osal_port_freertos_memory_get_minimum_free_size();
        LOGI("free heap size: %d", xFreeHeapSize);
        LOGI("min ever free heap size: %d", minEverFreeHeapSize);

        osal_task_delay(5000);
    }
}

void memory_monitor_thread_init(void) {
    osal_task_config_t config = {
        .name = "memory_monitor",
        .func = (osal_task_func_t)memory_monitor_thread,
        .param = NULL,
        .stack_size = 128 * sizeof(StackType_t),
        .priority = osalPriorityNormal,
    };
    if (OSAL_OK != osal_task_create(&config, &memory_thread_osal)) {
        LOGE("memory_monitor osal_task_create error");
    }
    osal_mutex_config_t mutex_config = {
        .name = "memory_mutex",
        .inherit = 1,
    };
    osal_mutex_create(&mutex_config, &memory_mutex);

    osal_queue_config_t queue_config = {
        .name = "memory_queue",
        .max_items = 10,
        .item_size = sizeof(uint32_t),
    };
    osal_queue_create(&queue_config, &memory_queue);
}

osal_task_t get_memory_moniter_thread_id(void) { return memory_thread_osal; }

void check_memory(void) {
    uint32_t prev_value;
    osal_task_notify_set(memory_thread_osal, 0X01, osal_SetBits, &prev_value);
}
