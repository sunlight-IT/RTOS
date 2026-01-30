/**
 * @file osal_sync.c
 * @brief OSAL同步组件实现（互斥锁、信号量、事件）
 * @details 通过函数指针调用适配层实现
 */

#include "osal.h"
#include "osal_ops.h"

/* ==================== 互斥锁操作接口实现 ==================== */

static const osal_mutex_ops_t* osal_get_mutex_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->mutex == NULL) {
        return NULL;
    }
    return ops->mutex;
}

osal_status_t osal_mutex_create(const osal_mutex_config_t* config, osal_mutex_t* handle) {
    const osal_mutex_ops_t* ops = osal_get_mutex_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->create == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_status_t ret = ops->create(config, handle);

    if (ret == OSAL_OK) {
        osal_instance_t* instance = osal_get_instance_internal();
        instance->mutex_count++;
    }

    return ret;
}

osal_status_t osal_mutex_delete(osal_mutex_t handle) {
    const osal_mutex_ops_t* ops = osal_get_mutex_ops();

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
        if (instance->mutex_count > 0) {
            instance->mutex_count--;
        }
    }

    return ret;
}

osal_status_t osal_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout) {
    const osal_mutex_ops_t* ops = osal_get_mutex_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->acquire == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->acquire(handle, timeout);
}

osal_status_t osal_mutex_release(osal_mutex_t handle) {
    const osal_mutex_ops_t* ops = osal_get_mutex_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->release == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->release(handle);
}



osal_status_t osal_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner) {
    const osal_mutex_ops_t* ops = osal_get_mutex_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_owner == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || owner == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_owner(handle, owner);
}

osal_status_t osal_mutex_get_info(osal_mutex_t handle, const char** name, uint8_t* inherit) {
    const osal_mutex_ops_t* ops = osal_get_mutex_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_info == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_info(handle, name, inherit);
}

osal_status_t osal_mutex_recursive_acquire(osal_mutex_t handle, osal_tick_t timeout) {
    /* 默认实现调用普通acquire */
    return osal_mutex_acquire(handle, timeout);
}

osal_status_t osal_mutex_recursive_release(osal_mutex_t handle) {
    /* 默认实现调用普通release */
    return osal_mutex_release(handle);
}

/* ==================== 信号量操作接口实现 ==================== */

static const osal_semaphore_ops_t* osal_get_semaphore_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->semaphore == NULL) {
        return NULL;
    }
    return ops->semaphore;
}

osal_status_t osal_semaphore_create(const osal_semaphore_config_t* config,
                                    osal_semaphore_t* handle) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

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
        instance->semaphore_count++;
    }

    return ret;
}

osal_status_t osal_semaphore_delete(osal_semaphore_t handle) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

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
        if (instance->semaphore_count > 0) {
            instance->semaphore_count--;
        }
    }

    return ret;
}

osal_status_t osal_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->acquire == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->acquire(handle, timeout);
}

osal_status_t osal_semaphore_release(osal_semaphore_t handle) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->release == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->release(handle);
}

osal_status_t osal_semaphore_get_count(osal_semaphore_t handle, uint32_t* count) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

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

osal_status_t osal_semaphore_set_count(osal_semaphore_t handle, uint32_t count) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->set_count == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->set_count(handle, count);
}

osal_status_t osal_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                      osal_semaphore_type_t* type, uint32_t* max_count) {
    const osal_semaphore_ops_t* ops = osal_get_semaphore_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_info == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_info(handle, name, type, max_count);
}

/* ==================== 事件操作接口实现 ==================== */

static const osal_event_ops_t* osal_get_event_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->event == NULL) {
        return NULL;
    }
    return ops->event;
}

osal_status_t osal_event_create(const osal_event_config_t* config, osal_event_t* handle) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->create == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_status_t ret = ops->create(config, handle);

    if (ret == OSAL_OK) {
        osal_instance_t* instance = osal_get_instance_internal();
        instance->event_count++;
    }

    return ret;
}

osal_status_t osal_event_delete(osal_event_t handle) {
    const osal_event_ops_t* ops = osal_get_event_ops();

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
        if (instance->event_count > 0) {
            instance->event_count--;
        }
    }

    return ret;
}

osal_status_t osal_event_wait(osal_event_t handle, osal_event_flags_t wait_flags, uint8_t option,
                              osal_event_flags_t* actual_flags, osal_tick_t timeout) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->wait == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->wait(handle, wait_flags, option, actual_flags, timeout);
}

osal_status_t osal_event_set(osal_event_t handle, osal_event_flags_t flags) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->set == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->set(handle, flags);
}



osal_status_t osal_event_clear(osal_event_t handle, osal_event_flags_t flags) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->clear == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->clear(handle, flags);
}

osal_status_t osal_event_get(osal_event_t handle, osal_event_flags_t* flags) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || flags == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get(handle, flags);
}

osal_status_t osal_event_sync(osal_event_t handle, osal_event_flags_t flags) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->sync == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->sync(handle, flags);
}

osal_status_t osal_event_get_info(osal_event_t handle, const char** name) {
    const osal_event_ops_t* ops = osal_get_event_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_info == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_info(handle, name);
}
