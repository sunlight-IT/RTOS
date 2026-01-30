/**
 * @file osal_task_port.c
 * @brief FreeRTOS任务组件适配实现
 * @details 实现FreeRTOS到OSAL的任务适配功能
 */

#include "osal_task_port.h"

#include <string.h>

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
    if (inHandlerMode()) {
        if (xTaskResumeFromISR(OSAL_TASK_HANDLE_TO_FREERTOS(handle)) == pdTRUE) {
            portYIELD_FROM_ISR(pdTRUE);
        }
    } else {
        vTaskResume(OSAL_TASK_HANDLE_TO_FREERTOS(handle));
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

    if (inHandlerMode()) {
        *priority = FREERTOS_PRIORITY_TO_OSAL(uxTaskPriorityGetFromISR(h));
    } else {
        *priority = FREERTOS_PRIORITY_TO_OSAL(uxTaskPriorityGet(h));
    }

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
