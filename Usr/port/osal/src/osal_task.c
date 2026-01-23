/**
 * @file osal_task.c
 * @brief OSAL任务组件实现
 * @details 通过函数指针调用适配层实现
 */

#include <string.h>

#include "osal.h"
#include "osal_ops.h"

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 获取任务操作接口
 */
static const osal_task_ops_t* osal_get_task_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->task == NULL) {
        return NULL;
    }
    return ops->task;
}

/* ==================== 任务操作接口实现 ==================== */

osal_status_t osal_task_create(const osal_task_config_t* config, osal_task_t* handle) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->create == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (config == NULL || handle == NULL || config->func == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_status_t ret = ops->create(config, handle);

    if (ret == OSAL_OK) {
        osal_instance_t* instance = osal_get_instance_internal();
        instance->task_count++;
    }

    return ret;
}

osal_status_t osal_task_delete(osal_task_t handle) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->delete == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_status_t ret = ops->delete(handle);

    if (ret == OSAL_OK) {
        osal_instance_t* instance = osal_get_instance_internal();
        if (instance->task_count > 0) {
            instance->task_count--;
        }
    }

    return ret;
}

osal_status_t osal_task_suspend(osal_task_t handle) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->suspend == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->suspend(handle);
}

osal_status_t osal_task_resume(osal_task_t handle) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->resume == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->resume(handle);
}

osal_task_t osal_task_get_current(void) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_current == NULL) {
        return NULL;
    }
#endif

    return ops->get_current();
}

osal_status_t osal_task_yield(void) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->yield == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
#endif

    return ops->yield();
}

osal_status_t osal_task_delay(osal_tick_t ticks) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->delay == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
#endif

    return ops->delay(ticks);
}

osal_status_t osal_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->delay_until == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (prev_tick == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->delay_until(prev_tick, ticks);
}

osal_status_t osal_task_get_priority(osal_task_t handle, osal_priority_t* priority) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_priority == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (priority == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_priority(handle, priority);
}

osal_status_t osal_task_set_priority(osal_task_t handle, osal_priority_t priority) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->set_priority == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
#endif

    return ops->set_priority(handle, priority);
}

osal_status_t osal_task_get_state(osal_task_t handle, osal_task_state_t* state) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_state == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || state == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_state(handle, state);
}

osal_status_t osal_task_get_info(osal_task_t handle, const char** name, uint32_t* stack_size,
                                 uint32_t* stack_free) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_info == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_info(handle, name, stack_size, stack_free);
}

osal_tick_t osal_task_get_tick_count(void) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_tick_count == NULL) {
        return 0;
    }
#endif

    return ops->get_tick_count();
}

int osal_task_is_in_isr(void) {
    const osal_task_ops_t* ops = osal_get_task_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->is_in_isr == NULL) {
        return 0;
    }
#endif

    return ops->is_in_isr();
}

osal_status_t osal_task_suspended(osal_task_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

uint32_t osal_task_get_count(void) {
    osal_instance_t* instance = osal_get_instance_internal();
    return instance->task_count;
}

uint32_t osal_task_get_list(osal_task_t* tasks, uint32_t max_count) {
    (void)tasks;
    (void)max_count;
    return 0;
}

/* ==================== 任务Hook函数实现 ==================== */

OSAL_WEAK void osal_task_create_hook(osal_task_t handle) { (void)handle; }

OSAL_WEAK void osal_task_delete_hook(osal_task_t handle) { (void)handle; }

OSAL_WEAK void osal_task_switch_hook(void) {}

OSAL_WEAK void osal_idle_task_hook(void) {}
