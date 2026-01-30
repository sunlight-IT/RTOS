/**
 * @file osal_task_notify.c
 * @brief OSAL任务通知组件实现
 * @details 通过函数指针调用适配层实现
 */

#include <stdlib.h>
#include <string.h>

#include "osal.h"
#include "osal_ops.h"

/* ==================== 内部辅助函数 ==================== */

static const osal_task_notify_ops_t* osal_get_task_notify_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->task_notify == NULL) {
        return NULL;
    }
    return ops->task_notify;
}

/* ==================== 任务通知操作接口实现 ==================== */

osal_status_t osal_task_notify_set(osal_task_t task, uint32_t value, uint8_t action,
                                   uint32_t* prev_value) {
    const osal_task_notify_ops_t* ops = osal_get_task_notify_ops();
    if (ops == NULL || ops->notify == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }

    return ops->notify(task, value, action, prev_value);
}

osal_status_t osal_task_notify_wait(uint32_t clear_bits_entry, uint32_t clear_bits_exit,
                                    uint32_t* value, osal_tick_t timeout) {
    const osal_task_notify_ops_t* ops = osal_get_task_notify_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->wait == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
#endif

    return ops->wait(clear_bits_entry, clear_bits_exit, value, timeout);
}

osal_status_t osal_task_notify_clear(osal_task_t task, uint32_t bits_to_clear,
                                     uint32_t* prev_value) {
    const osal_task_notify_ops_t* ops = osal_get_task_notify_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->clear == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
#endif

    return ops->clear(task, bits_to_clear, prev_value);
}

/* ==================== Hook函数实现 ==================== */

OSAL_WEAK void osal_task_notify_send_hook(osal_task_t task, uint32_t value) {
    (void)task;
    (void)value;
    /* 用户可重写 */
}

OSAL_WEAK void osal_task_notify_receive_hook(uint32_t value) {
    (void)value;
    /* 用户可重写 */
}
