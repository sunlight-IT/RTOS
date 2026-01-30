/**
 * @file osal_semaphore_port.c
 * @brief FreeRTOS信号量组件适配实现
 * @details 实现FreeRTOS到OSAL的信号量适配功能
 */

#include "osal_semaphore_port.h"

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

    if (inHandlerMode()) {
        return OSAL_ERROR_ISR;
    }

    vSemaphoreDelete(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle));
    return OSAL_OK;
}

osal_status_t osal_port_freertos_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout) {
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
        ret = xSemaphoreTakeFromISR(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle), &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                           : (timeout == OSAL_NO_WAIT)    ? 0
                                                          : OSAL_TICKS_TO_FREERTOS(timeout);

        ret = xSemaphoreTake(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle), ticks);
    }

    return freertos_to_osal_status(ret);
}

osal_status_t osal_port_freertos_semaphore_release(osal_semaphore_t handle) {
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
        ret = xSemaphoreGiveFromISR(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle), &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        ret = xSemaphoreGive(OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle));
    }

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
