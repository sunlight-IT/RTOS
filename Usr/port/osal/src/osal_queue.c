/**
 * @file osal_queue.c
 * @brief OSAL队列组件实现
 * @details 通过函数指针调用适配层实现
 */

#include "osal.h"
#include "osal_ops.h"

/* ==================== 内部辅助函数 ==================== */

static const osal_queue_ops_t* osal_get_queue_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->queue == NULL) {
        return NULL;
    }
    return ops->queue;
}

/* ==================== 队列操作接口实现 ==================== */

osal_status_t osal_queue_create(const osal_queue_config_t* config, osal_queue_t* handle) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->create == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (config == NULL || handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_status_t ret = ops->create(config, handle);

    if (ret == OSAL_OK) {
        osal_instance_t* instance = osal_get_instance_internal();
        instance->queue_count++;
    }

    return ret;
}

osal_status_t osal_queue_delete(osal_queue_t handle) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

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
        if (instance->queue_count > 0) {
            instance->queue_count--;
        }
    }

    return ret;
}

osal_status_t osal_queue_send(osal_queue_t handle, const void* data, osal_tick_t timeout) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->send == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->send(handle, data, timeout);
}

osal_status_t osal_queue_send_front(osal_queue_t handle, const void* data, osal_tick_t timeout) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->send_front == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->send_front(handle, data, timeout);
}

osal_status_t osal_queue_receive(osal_queue_t handle, void* data, osal_tick_t timeout) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->receive == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->receive(handle, data, timeout);
}

osal_status_t osal_queue_reset(osal_queue_t handle) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->reset == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->reset(handle);
}

osal_status_t osal_queue_get_count(osal_queue_t handle, uint32_t* count) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_count == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || count == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_count(handle, count);
}

osal_status_t osal_queue_get_space(osal_queue_t handle, uint32_t* space) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_space == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || space == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_space(handle, space);
}

osal_status_t osal_queue_is_empty(osal_queue_t handle, int* is_empty) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->is_empty == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || is_empty == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->is_empty(handle, is_empty);
}

osal_status_t osal_queue_is_full(osal_queue_t handle, int* is_full) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->is_full == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || is_full == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->is_full(handle, is_full);
}

osal_status_t osal_queue_get_info(osal_queue_t handle, const char** name, uint32_t* max_items,
                                  uint32_t* item_size) {
    const osal_queue_ops_t* ops = osal_get_queue_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_info == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_info(handle, name, max_items, item_size);
}

osal_status_t osal_queue_traverse(osal_queue_t handle, osal_queue_traverse_cb_t callback,
                                  void* context) {
    (void)handle;
    (void)callback;
    (void)context;
    return OSAL_ERROR_NOT_SUPPORTED;
}
