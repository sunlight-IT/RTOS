/**
 * @file osal_event_port.c
 * @brief FreeRTOS事件组件适配实现
 * @details 实现FreeRTOS到OSAL的事件适配功能
 */

#include "osal_event_port.h"

#include <string.h>

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

    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                       : (timeout == OSAL_NO_WAIT)    ? 0
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

    BaseType_t taskWoken = pdFALSE;
    BaseType_t ret;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本 */

        if (xEventGroupSetBitsFromISR(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags, &taskWoken) !=
            pdTRUE) {
            return OSAL_ERROR_ISR;
        }
        portEND_SWITCHING_ISR(taskWoken);

    } else {
        /* 普通模式：正常调用 */
        if ((ret = xEventGroupSetBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags)) != pdTRUE) {
            return OSAL_ERROR;
        }
    }
    return OSAL_OK;
}

osal_status_t osal_port_freertos_event_clear(osal_event_t handle, osal_event_flags_t flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    BaseType_t taskWoken = pdFALSE;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本 */
        if (xEventGroupClearBitsFromISR(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags) != pdTRUE) {
            return OSAL_ERROR_ISR;
        }
        portEND_SWITCHING_ISR(taskWoken);
    } else {
        /* 普通模式：正常调用 */
        if (xEventGroupClearBits(OSAL_EVENT_HANDLE_TO_FREERTOS(handle), flags) != pdTRUE) {
            return OSAL_ERROR;
        }
    }

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
