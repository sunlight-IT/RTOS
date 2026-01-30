/**
 * @file osal_queue_port.c
 * @brief FreeRTOS队列组件适配实现
 * @details 实现FreeRTOS到OSAL的队列适配功能
 */

#include "osal_queue_port.h"

#include <string.h>

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief FreeRTOS错误码转换为OSAL错误码
 */
static osal_status_t freertos_to_osal_status(BaseType_t freertos_status) {
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

    if (inHandlerMode()) {
        return OSAL_ERROR_ISR;
    }

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

    BaseType_t taskWoken = pdFALSE;
    BaseType_t ret;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本，不使用timeout */
        ret = xQueueSendFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                           : (timeout == OSAL_NO_WAIT)    ? 0
                                                          : OSAL_TICKS_TO_FREERTOS(timeout);

        ret = xQueueSend(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, ticks);
    }

    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_send_front(osal_queue_t handle, const void* data,
                                                  osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t taskWoken = pdFALSE;
    BaseType_t ret;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本，不使用timeout */
        ret = xQueueSendToFrontFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                           : (timeout == OSAL_NO_WAIT)    ? 0
                                                          : OSAL_TICKS_TO_FREERTOS(timeout);

        ret = xQueueSendToFront(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, ticks);
    }
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_queue_receive(osal_queue_t handle, void* data,
                                               osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t taskWoken = pdFALSE;
    BaseType_t ret;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本，不使用timeout */
        ret = xQueueReceiveFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                           : (timeout == OSAL_NO_WAIT)    ? 0
                                                          : OSAL_TICKS_TO_FREERTOS(timeout);

        ret = xQueueReceive(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle), data, ticks);
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

    if (inHandlerMode()) {
        *count = uxQueueMessagesWaitingFromISR(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle));
    } else {
        *count = uxQueueMessagesWaiting(OSAL_QUEUE_HANDLE_TO_FREERTOS(handle));
    }
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

    /* FreeRTOS没有直接获取队列容量的API，返回0 */
    if (max_items != NULL) {
        *max_items = 0;
    }

    if (item_size != NULL) {
        *item_size = 0;
    }

    return OSAL_OK;
}
