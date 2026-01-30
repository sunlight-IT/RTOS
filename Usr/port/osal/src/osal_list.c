/**
 * @file osal_list.c
 * @brief OSAL链表组件实现
 * @details 通过函数指针调用适配层实??
 */

#include <stdlib.h>
#include <string.h>

#include "osal.h"
#include "osal_ops.h"

/* ==================== 内部辅助函数 ==================== */

static const osal_list_ops_t* osal_get_list_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->list == NULL) {
        return NULL;
    }
    return ops->list;
}

/* ==================== 链表操作接口实现 ==================== */

osal_status_t osal_list_create(const osal_list_config_t* config, osal_list_t* handle) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->create == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (config == NULL || handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->create(config, handle);
}

osal_status_t osal_list_delete(osal_list_t handle) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->del == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->del(handle);
}

osal_status_t osal_list_push_front(osal_list_t handle, const void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->push_front == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->push_front(handle, data);
}

osal_status_t osal_list_push_back(osal_list_t handle, const void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->push_back == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->push_back(handle, data);
}

osal_status_t osal_list_insert_after(osal_list_t handle, osal_list_node_t* position,
                                     const void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->insert_after == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->insert_after(handle, position, data);
}

osal_status_t osal_list_insert_before(osal_list_t handle, osal_list_node_t* position,
                                      const void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->insert_before == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->insert_before(handle, position, data);
}

osal_status_t osal_list_pop_front(osal_list_t handle, void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->pop_front == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->pop_front(handle, data);
}

osal_status_t osal_list_pop_back(osal_list_t handle, void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->pop_back == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->pop_back(handle, data);
}

osal_status_t osal_list_remove(osal_list_t handle, osal_list_node_t* node, void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->remove == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->remove(handle, node, data);
}

osal_status_t osal_list_front(osal_list_t handle, osal_list_node_t** node) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->front == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->front(handle, node);
}

osal_status_t osal_list_back(osal_list_t handle, osal_list_node_t** node) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->back == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->back(handle, node);
}

osal_status_t osal_list_next(osal_list_node_t* node, osal_list_node_t** next) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->next == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (node == NULL || next == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->next(node, next);
}

osal_status_t osal_list_prev(osal_list_node_t* node, osal_list_node_t** prev) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->prev == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (node == NULL || prev == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->prev(node, prev);
}

osal_status_t osal_list_get_data(osal_list_node_t* node, void** data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_data == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (node == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_data(node, data);
}

osal_status_t osal_list_set_data(osal_list_node_t* node, const void* data) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->set_data == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (node == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->set_data(node, data);
}

osal_status_t osal_list_get_count(osal_list_t handle, uint32_t* count) {
    const osal_list_ops_t* ops = osal_get_list_ops();

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

osal_status_t osal_list_is_empty(osal_list_t handle, int* is_empty) {
    const osal_list_ops_t* ops = osal_get_list_ops();

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

osal_status_t osal_list_clear(osal_list_t handle) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->clear == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->clear(handle);
}

osal_status_t osal_list_reverse(osal_list_t handle) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->reverse == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->reverse(handle);
}

osal_status_t osal_list_sort(osal_list_t handle, osal_list_compare_cb_t compare) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->sort == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || compare == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->sort(handle, compare);
}

osal_status_t osal_list_find(osal_list_t handle, const void* data, osal_list_compare_cb_t compare,
                             osal_list_node_t** node) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->find == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || data == NULL || compare == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->find(handle, data, compare, node);
}

osal_status_t osal_list_traverse(osal_list_t handle, osal_list_traverse_cb_t callback,
                                 void* context) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->traverse == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || callback == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->traverse(handle, callback, context);
}

osal_status_t osal_list_get_info(osal_list_t handle, const char** name, uint32_t* count,
                                 uint32_t* max_nodes) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_info == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->get_info(handle, name, count, max_nodes);
}

osal_status_t osal_list_at(osal_list_t handle, uint32_t index, osal_list_node_t** node) {
    const osal_list_ops_t* ops = osal_get_list_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->at == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->at(handle, index, node);
}

/* ==================== Hook函数实现 ==================== */

OSAL_WEAK void osal_list_create_hook(osal_list_t handle) {
    (void)handle;
    /* 用户??重写 */
}

OSAL_WEAK void osal_list_delete_hook(osal_list_t handle) {
    (void)handle;
    /* 用户??重写 */
}
