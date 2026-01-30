/**
 * @file osal_notify_port.c
 * @brief FreeRTOS任务通知组件适配实现
 * @details 实现FreeRTOS到OSAL的任务通知适配功能
 */

#include "osal_notify_port.h"

#include <string.h>

/* ==================== 任务通知适配实现 ==================== */

osal_status_t osal_port_freertos_task_notify(osal_task_t task, uint32_t value, uint8_t action,
                                             uint32_t* prev_value) {
    TaskHandle_t h =
        (task != NULL) ? OSAL_TASK_HANDLE_TO_FREERTOS(task) : xTaskGetCurrentTaskHandle();
    BaseType_t ret;

    /* 检测是否在中断模式下 */
    if (inHandlerMode()) {
        /* 中断模式：调用FromISR版本 */
        BaseType_t taskWoken = pdFALSE;
        eNotifyAction eAction = (eNotifyAction)action;
        ret = xTaskNotifyAndQueryFromISR(h, value, eAction, (uint32_t*)prev_value, &taskWoken);
        if (ret == pdTRUE) {
            portEND_SWITCHING_ISR(taskWoken);
        }
    } else {
        /* 普通模式：正常调用 */
        eNotifyAction eAction = (eNotifyAction)action;
        ret = xTaskNotifyAndQuery(h, value, eAction, (uint32_t*)prev_value);
    }

    return (ret == pdPASS) ? OSAL_OK : OSAL_ERROR;
}

osal_status_t osal_port_freertos_task_notify_wait(uint32_t clear_bits_entry,
                                                  uint32_t clear_bits_exit, uint32_t* value,
                                                  osal_tick_t timeout) {
    TickType_t ticks = (timeout == OSAL_WAIT_FOREVER) ? portMAX_DELAY
                       : (timeout == OSAL_NO_WAIT)    ? 0
                                                      : OSAL_TICKS_TO_FREERTOS(timeout);
    BaseType_t ret = pdFAIL;

    if (inHandlerMode()) {
        ret = OSAL_ERROR_ISR; /* Not allowed in ISR */
    } else if (pdTRUE !=
               xTaskNotifyWait(clear_bits_entry, clear_bits_exit, (uint32_t*)value, ticks)) {
        if (ticks == 0) {
            ret = OSAL_OK;
        } else {
            ret = OSAL_ERROR_TIMEOUT;
        }
    } else {
        ret = OSAL_OK;
    }

    return ret;
}

osal_status_t osal_port_freertos_task_notify_clear(osal_task_t task, uint32_t bits_to_clear,
                                                   uint32_t* prev_value) {
    TaskHandle_t h = OSAL_TASK_HANDLE_TO_FREERTOS(task);
    uint32_t current_value;
    BaseType_t ret = pdFAIL;

    current_value = ulTaskNotifyValueClear(h, bits_to_clear);
    if (prev_value != NULL) {
        *prev_value = current_value;
        ret = pdPASS;
    }

    return (ret == pdPASS) ? OSAL_OK : OSAL_ERROR;
}
