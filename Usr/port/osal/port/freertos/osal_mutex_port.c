/**
 * @file osal_mutex_port.c
 * @brief FreeRTOS互斥锁组件适配实现
 * @details 实现FreeRTOS到OSAL的互斥锁适配功能
 */

#include "osal_mutex_port.h"

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

    if (inHandlerMode()) {
        return OSAL_ERROR_ISR;
    }

    vSemaphoreDelete(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t taskWoken = pdFALSE;
    BaseType_t ret;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本，不使用timeout */
        ret = xSemaphoreTakeFromISR(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle), &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                           : (timeout == OSAL_NO_WAIT)    ? 0
                                                          : OSAL_TICKS_TO_FREERTOS(timeout);

        ret = xSemaphoreTake(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle), ticks);
    }
    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_mutex_release(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t taskWoken = pdFALSE;
    BaseType_t ret;
    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本 */
        ret = xSemaphoreGiveFromISR(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle), &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        ret = xSemaphoreGive(OSAL_MUTEX_HANDLE_TO_FREERTOS(handle));
    }
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
