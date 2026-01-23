/**
 * @file osal_port.c
 * @brief FreeRTOS适配层实现
 * @details 实现FreeRTOS到OSAL的适配功能
 */

#include "osal_port.h"

#include <stdlib.h>
#include <string.h>

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief FreeRTOS错误码转换为OSAL错误码
 */
OSAL_INLINE osal_status_t freertos_to_osal_status(BaseType_t freertos_status) {
    switch (freertos_status) {
        case pdPASS:
            return OSAL_OK;
        case errQUEUE_FULL:
            return OSAL_ERROR_TIMEOUT;
        case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
            return OSAL_ERROR_NO_MEM;
        default:
            return OSAL_ERROR;
    }
}

/* ==================== 初始化和反初始化 ==================== */

osal_status_t osal_port_freertos_init(void) {
    /* FreeRTOS已经在vTaskStartScheduler之前初始化 */
    return OSAL_OK;
}

osal_status_t osal_port_freertos_deinit(void) {
    /* FreeRTOS由vTaskEndScheduler处理 */
    return OSAL_OK;
}

/* ==================== 任务适配实现 ==================== */

osal_status_t osal_port_freertos_task_create(const osal_task_config_t* config,
                                             osal_task_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL || config->func == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t ret = xTaskCreate(
        (TaskFunction_t)config->func, config->name != NULL ? config->name : "OSAL_Task",
        config->stack_size / sizeof(StackType_t), /* FreeRTOS使用字作为栈单位 */
        config->param, OSAL_PRIORITY_TO_FREERTOS(config->priority), (TaskHandle_t*)handle);

    return (ret == pdPASS) ? OSAL_OK : OSAL_ERROR_NO_MEM;
}

osal_status_t osal_port_freertos_task_delete(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vTaskDelete(OSAL_TASK_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_suspend(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vTaskSuspend(OSAL_TASK_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_resume(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    if (xTaskResumeFromISR(OSAL_TASK_HANDLE_TO_FREERTOS(handle)) == pdTRUE) {
        taskYIELD();
    }
    return OSAL_OK;
}

osal_task_t osal_port_freertos_task_get_current(void) {
    return FREERTOS_TASK_HANDLE_TO_OSAL(xTaskGetCurrentTaskHandle());
}

osal_status_t osal_port_freertos_task_yield(void) {
    taskYIELD();
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_delay(osal_tick_t ticks) {
    vTaskDelay(OSAL_TICKS_TO_FREERTOS(ticks));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks) {
#if OSAL_CFG_PARAM_CHECK
    if (prev_tick == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t prev = OSAL_TICKS_TO_FREERTOS(*prev_tick);
    vTaskDelayUntil(&prev, OSAL_TICKS_TO_FREERTOS(ticks));
    *prev_tick = FREERTOS_TICKS_TO_OSAL(prev);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_get_priority(osal_task_t handle, osal_priority_t* priority) {
#if OSAL_CFG_PARAM_CHECK
    if (priority == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TaskHandle_t h =
        (handle != NULL) ? OSAL_TASK_HANDLE_TO_FREERTOS(handle) : xTaskGetCurrentTaskHandle();
    *priority = FREERTOS_PRIORITY_TO_OSAL(uxTaskPriorityGet(h));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_set_priority(osal_task_t handle, osal_priority_t priority) {
    TaskHandle_t h =
        (handle != NULL) ? OSAL_TASK_HANDLE_TO_FREERTOS(handle) : xTaskGetCurrentTaskHandle();
    vTaskPrioritySet(h, OSAL_PRIORITY_TO_FREERTOS(priority));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_get_state(osal_task_t handle, osal_task_state_t* state) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || state == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    eTaskState freertos_state = eTaskGetState(OSAL_TASK_HANDLE_TO_FREERTOS(handle));

    switch (freertos_state) {
        case eReady:
            *state = OSAL_TASK_STATE_READY;
            break;
        case eRunning:
            *state = OSAL_TASK_STATE_RUNNING;
            break;
        case eBlocked:
            *state = OSAL_TASK_STATE_BLOCKED;
            break;
        case eSuspended:
            *state = OSAL_TASK_STATE_SUSPENDED;
            break;
        case eDeleted:
            *state = OSAL_TASK_STATE_DELETED;
            break;
        default:
            *state = OSAL_TASK_STATE_READY;
            break;
    }

    return OSAL_OK;
}

osal_status_t osal_port_freertos_task_get_info(osal_task_t handle, const char** name,
                                               uint32_t* stack_size, uint32_t* stack_free) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TaskHandle_t h = OSAL_TASK_HANDLE_TO_FREERTOS(handle);

    if (name != NULL) {
        *name = pcTaskGetName(h);
    }

    if (stack_size != NULL) {
        *stack_size = uxTaskGetStackHighWaterMark(h) * sizeof(StackType_t);
    }

    if (stack_free != NULL) {
        *stack_free = uxTaskGetStackHighWaterMark(h) * sizeof(StackType_t);
    }

    return OSAL_OK;
}

osal_tick_t osal_port_freertos_task_get_tick_count(void) {
    return FREERTOS_TICKS_TO_OSAL(xTaskGetTickCount());
}

int osal_port_freertos_task_is_in_isr(void) { return (xPortIsInsideInterrupt() != 0); }

/* ==================== 队列适配实现 ==================== */

osal_status_t osal_port_freertos_queue_create(const osal_queue_config_t* config,
                                              osal_queue_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL || config->item_size == 0 || config->max_items == 0) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    QueueHandle_t queue = xQueueCreate(config->max_items, config->item_size);
    if (queue == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = FREERTOS_QUEUE_HANDLE_TO_OSAL(queue);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_delete(osal_queue_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vQueueDelete(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_send(osal_queue_t handle, const void* data,
                                            osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_FREERTOS_FOREVER
                       : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_FREERTOS_NO_WAIT
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);

    BaseType_t ret = xQueueSend(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, ticks);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_send_front(osal_queue_t handle, const void* data,
                                                  osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_FREERTOS_FOREVER
                       : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_FREERTOS_NO_WAIT
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);

    BaseType_t ret = xQueueSendToFront(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, ticks);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_receive(osal_queue_t handle, void* data,
                                               osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_FREERTOS_FOREVER
                       : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_FREERTOS_NO_WAIT
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);

    BaseType_t ret = xQueueReceive(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, ticks);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_send_from_isr(osal_queue_t handle, const void* data,
                                                     int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t woken = pdFALSE;
    BaseType_t ret = xQueueSendFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, &woken);

    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (woken == pdTRUE) ? 1 : 0;
    }

    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_receive_from_isr(osal_queue_t handle, void* data,
                                                        int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t woken = pdFALSE;
    BaseType_t ret = xQueueReceiveFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, &woken);

    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (woken == pdTRUE) ? 1 : 0;
    }

    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_reset(osal_queue_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    xQueueReset(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_get_count(osal_queue_t handle, uint32_t* count) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || count == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    *count = uxQueueMessagesWaiting(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_get_space(osal_queue_t handle, uint32_t* space) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || space == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    *space = uxQueueSpacesAvailable(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_is_empty(osal_queue_t handle, int* is_empty) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || is_empty == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    *is_empty = xQueueIsQueueEmptyFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle)) ? 1 : 0;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_is_full(osal_queue_t handle, int* is_full) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || is_full == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    *is_full = xQueueIsQueueFullFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle)) ? 1 : 0;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_queue_get_info(osal_queue_t handle, const char** name,
                                                uint32_t* max_items, uint32_t* item_size) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    QueueHandle_t h = OSAL_QUEUE_HANDLE_TO_FREERTOS(handle);

    if (name != NULL) {
        *name = pcQueueGetName(h);
    }

    if (max_items != NULL) {
        *max_items = uxQueueGetQueueNumber(h);
    }

    if (item_size != NULL) {
        *item_size = uxQueueGetQueueNumber(h);
    }

    return OSAL_OK;
}

/* ==================== 互斥锁适配实现 ==================== */

osal_status_t osal_port_freertos_mutex_create(const osal_mutex_config_t* config,
                                              osal_mutex_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    SemaphoreHandle_t mutex;
    if (config != NULL && config->inherit) {
        mutex = xSemaphoreCreateRecursiveMutex();
    } else {
        mutex = xSemaphoreCreateMutex();
    }

    if (mutex == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = FREERTOS_MUTEX_HANDLE_TO_OSAL(mutex);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_mutex_delete(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vSemaphoreDelete(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_FREERTOS_FOREVER
                       : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_FREERTOS_NO_WAIT
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);

    BaseType_t ret = xSemaphoreTake(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle), ticks);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_mutex_release(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t ret = xSemaphoreGive(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle));
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_mutex_try_acquire(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t ret =
        xSemaphoreTake(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle), OSAL_WAIT_FREERTOS_NO_WAIT);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || owner == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TaskHandle_t task = xSemaphoreGetMutexHolder(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle));
    *owner = FREERTOS_TASK_HANDLE_TO_OSAL(task);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_mutex_get_info(osal_mutex_t handle, const char** name,
                                                uint8_t* inherit) {
    (void)handle;  /* FreeRTOS互斥锁没有名称 */
    (void)inherit; /* 继承信息在创建时决定，无法查询 */

    if (name != NULL) {
        *name = "freertos_mutex";
    }

    return OSAL_OK;
}

/* ==================== 信号量适配实现 ==================== */

osal_status_t osal_port_freertos_semaphore_create(const osal_semaphore_config_t* config,
                                                  osal_semaphore_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    SemaphoreHandle_t sem;

    if (config->type == OSAL_SEMAPHORE_BINARY) {
        sem = xSemaphoreCreateBinary();
    } else {
        sem = xSemaphoreCreateCounting(config->max_count, config->init_count);
    }

    if (sem == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = FREERTOS_SEMAPHORE_HANDLE_TO_OSAL(sem);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_semaphore_delete(osal_semaphore_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vSemaphoreDelete(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_FREERTOS_FOREVER
                       : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_FREERTOS_NO_WAIT
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);

    BaseType_t ret = xSemaphoreTake(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle), ticks);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_semaphore_release(osal_semaphore_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t ret = xSemaphoreGive(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle));
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_semaphore_release_from_isr(osal_semaphore_t handle,
                                                            int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t woken = pdFALSE;
    BaseType_t ret = xSemaphoreGiveFromISR(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle), &woken);

    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (woken == pdTRUE) ? 1 : 0;
    }

    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_semaphore_try_acquire(osal_semaphore_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t ret =
        xSemaphoreTake(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle), OSAL_WAIT_FREERTOS_NO_WAIT);
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_semaphore_get_count(osal_semaphore_t handle, uint32_t* count) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || count == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    *count = uxSemaphoreGetCount(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_semaphore_set_count(osal_semaphore_t handle, uint32_t count) {
    /* FreeRTOS不支持直接设置计数值 */
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_freertos_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                                    osal_semaphore_type_t* type,
                                                    uint32_t* max_count) {
    (void)handle; /* FreeRTOS信号量没有名称和类型信息 */

    if (name != NULL) {
        *name = "freertos_semaphore";
    }

    if (type != NULL) {
        *type = OSAL_SEMAPHORE_BINARY; /* 默认为二值信号量 */
    }

    if (max_count != NULL) {
        *max_count = 1;
    }

    return OSAL_OK;
}

/* ==================== 事件适配实现 ==================== */

osal_status_t osal_port_freertos_event_create(const osal_event_config_t* config,
                                              osal_event_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    (void)config; /* FreeRTOS事件组不需要配置参数 */

    EventGroupHandle_t event = xEventGroupCreate();
    if (event == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = FREERTOS_EVENT_HANDLE_TO_OSAL(event);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_delete(osal_event_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vEventGroupDelete(OSAL_EVENT_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_wait(osal_event_t handle, osal_event_flags_t wait_flags,
                                            uint8_t option, osal_event_flags_t* actual_flags,
                                            osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_FREERTOS_FOREVER
                       : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_FREERTOS_NO_WAIT
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);

    BaseType_t wait_all = (option & OSAL_EVENT_WAIT_ALL) ? pdTRUE : pdFALSE;
    BaseType_t clear = (option & OSAL_EVENT_WAIT_CLEAR) ? pdTRUE : pdFALSE;

    EventBits_t bits = xEventGroupWaitBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), wait_flags, clear,
                                           wait_all, ticks);

    if (actual_flags != NULL) {
        *actual_flags = bits;
    }

    /* 检查是否所有等待的标志都已设置 */
    if (wait_all) {
        return ((bits & wait_flags) == wait_flags) ? OSAL_OK : OSAL_ERROR_TIMEOUT;
    } else {
        return ((bits & wait_flags) != 0) ? OSAL_OK : OSAL_ERROR_TIMEOUT;
    }
}

osal_status_t osal_port_freertos_event_set(osal_event_t handle, osal_event_flags_t flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    xEventGroupSetBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_set_from_isr(osal_event_t handle, osal_event_flags_t flags,
                                                    int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t woken = pdFALSE;
    xEventGroupSetBitsFromISR(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags, &woken);

    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (woken == pdTRUE) ? 1 : 0;
    }

    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_clear(osal_event_t handle, osal_event_flags_t flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    xEventGroupClearBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_get(osal_event_t handle, osal_event_flags_t* flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || flags == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    *flags = xEventGroupGetBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_sync(osal_event_t handle, osal_event_flags_t flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    xEventGroupSetBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_get_info(osal_event_t handle, const char** name) {
    (void)handle; /* FreeRTOS事件组没有名称 */

    if (name != NULL) {
        *name = "freertos_event";
    }

    return OSAL_OK;
}

/* ==================== 内存管理适配实现 ==================== */

void* osal_port_freertos_memory_alloc(uint32_t size) { return pvPortMalloc(size); }

osal_status_t osal_port_freertos_memory_free(void* ptr) {
#if OSAL_CFG_PARAM_CHECK
    if (ptr == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vPortFree(ptr);
    return OSAL_OK;
}

void* osal_port_freertos_memory_realloc(void* ptr, uint32_t size) {
    /* FreeRTOS标准没有realloc，使用自定义实现 */
    void* new_ptr = pvPortMalloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    if (ptr != NULL) {
        /* 获取原大小（FreeRTOS没有直接方式，这里假设用户需要手动复制） */
        memcpy(new_ptr, ptr, size); /* 简化处理，实际应用可能需要更复杂逻辑 */
        vPortFree(ptr);
    }

    return new_ptr;
}

void* osal_port_freertos_memory_calloc(uint32_t num, uint32_t size) {
    void* ptr = pvPortMalloc(num * size);
    if (ptr != NULL) {
        memset(ptr, 0, num * size);
    }
    return ptr;
}

uint32_t osal_port_freertos_memory_get_free_size(void) { return xPortGetFreeHeapSize(); }

uint32_t osal_port_freertos_memory_get_minimum_free_size(void) {
    return xPortGetMinimumEverFreeHeapSize();
}

/* ==================== 调度器控制适配实现 ==================== */

osal_status_t osal_port_freertos_scheduler_suspend(void) {
    vTaskSuspendAll();
    return OSAL_OK;
}

osal_status_t osal_port_freertos_scheduler_resume(void) {
    if (xTaskResumeAll() == pdTRUE) {
        taskYIELD();
    }
    return OSAL_OK;
}
