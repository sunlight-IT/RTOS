/**
 * @file osal_list_port.c
 * @brief FreeRTOS链表组件适配实现
 * @details 使用FreeRTOS原生链表组件实现OSAL链表功能
 * @note 适配FreeRTOS的List_t/ListItem_t到OSAL的链表接口
 */

#include "osal_list_port.h"

#include <string.h>

/* ==================== 内部辅助函数 ==================== */

#define listGET_PREV(pxListItem) ((pxListItem)->pxPrevious)
/**
 * @brief 创建新的链表节点
 * @param data 数据指针
 * @param node_size 数据大小
 * @return 新创建的节点指针，失败返回NULL
 */
static ListItem_t* list_node_create(const void* data, uint32_t node_size) {
    ListItem_t* item = (ListItem_t*)pvPortMalloc(sizeof(ListItem_t));
    if (item == NULL) {
        return NULL;
    }

    /* 分配数据空间并复制数据 */
    void* data_copy = pvPortMalloc(node_size);
    if (data_copy == NULL) {
        vPortFree(item);
        return NULL;
    }
    memcpy(data_copy, data, node_size);

    /* 初始化FreeRTOS列表项 */
    vListInitialiseItem(item);

    /* 将数据指针存储在pvOwner中 */
    listSET_LIST_ITEM_OWNER(item, data_copy);

    return item;
}

/**
 * @brief 释放链表节点
 * @param item 要释放的节点
 */
static void list_node_free(ListItem_t* item) {
    if (item != NULL) {
        /* 释放存储的数据 */
        void* data = listGET_LIST_ITEM_OWNER(item);
        if (data != NULL) {
            vPortFree(data);
        }
        vPortFree(item);
    }
}

/* ==================== 链表适配实现 ==================== */

osal_status_t osal_port_freertos_list_create(const osal_list_config_t* config,
                                             osal_list_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    /* 分配包装结构 */
    osal_freertos_list_t* wrapper =
        (osal_freertos_list_t*)pvPortMalloc(sizeof(osal_freertos_list_t));
    if (wrapper == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    /* 分配FreeRTOS原生链表 */
    wrapper->list = (List_t*)pvPortMalloc(sizeof(List_t));
    if (wrapper->list == NULL) {
        vPortFree(wrapper);
        return OSAL_ERROR_NO_MEM;
    }

    /* 初始化FreeRTOS链表 */
    vListInitialise(wrapper->list);

    wrapper->name = (config->name != NULL) ? config->name : "freertos_list";
    wrapper->max_nodes = config->max_nodes;
    wrapper->node_size = config->node_size;

    *handle = (osal_list_t)wrapper;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_delete(osal_list_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    /* 清空链表：遍历并释放所有节点 */
    ListItem_t* item = listGET_HEAD_ENTRY(list);
    while (item != listGET_END_MARKER(list)) {
        ListItem_t* next = listGET_NEXT(item);
        uxListRemove(item);
        list_node_free(item);
        item = next;
    }

    vPortFree(list);
    vPortFree(wrapper);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_push_front(osal_list_t handle, const void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    /* 检查节点数量限制 */
    if (wrapper->max_nodes > 0 && listCURRENT_LIST_LENGTH(list) >= wrapper->max_nodes) {
        return OSAL_ERROR_MAX_COUNT;
    }

    /* 创建新节点 */
    ListItem_t* item = list_node_create(data, wrapper->node_size);
    if (item == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    /* 插入到链表头部（使用最大值确保在头部） */
    listSET_LIST_ITEM_VALUE(item, portMAX_DELAY);
    vListInsert(list, item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_push_back(osal_list_t handle, const void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    /* 检查节点数量限制 */
    if (wrapper->max_nodes > 0 && listCURRENT_LIST_LENGTH(list) >= wrapper->max_nodes) {
        return OSAL_ERROR_MAX_COUNT;
    }

    /* 创建新节点 */
    ListItem_t* item = list_node_create(data, wrapper->node_size);
    if (item == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    /* 插入到链表尾部 */
    vListInsertEnd(list, item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_insert_after(osal_list_t handle, osal_list_node_t* position,
                                                   const void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;
    ListItem_t* pos = (ListItem_t*)position;

    /* 检查节点数量限制 */
    if (wrapper->max_nodes > 0 && listCURRENT_LIST_LENGTH(list) >= wrapper->max_nodes) {
        return OSAL_ERROR_MAX_COUNT;
    }

    /* 创建新节点 */
    ListItem_t* item = list_node_create(data, wrapper->node_size);
    if (item == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    /* 插入到position后面 */
    /* FreeRTOS链表按xItemValue排序，我们使用相邻值的策略 */
    TickType_t pos_value = listGET_LIST_ITEM_VALUE(pos);
    TickType_t next_value = pos_value;
    ListItem_t* next_item = listGET_NEXT(pos);

    /* 找到合适的值 */
    if (next_item != listGET_END_MARKER(list)) {
        TickType_t next_item_value = listGET_LIST_ITEM_VALUE(next_item);
        if (next_item_value > pos_value) {
            next_value = next_item_value;
        }
    }

    /* 设置值介于两者之间 */
    listSET_LIST_ITEM_VALUE(item, pos_value + 1);
    vListInsert(list, item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_insert_before(osal_list_t handle, osal_list_node_t* position,
                                                    const void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;
    ListItem_t* pos = (ListItem_t*)position;

    /* 检查节点数量限制 */
    if (wrapper->max_nodes > 0 && listCURRENT_LIST_LENGTH(list) >= wrapper->max_nodes) {
        return OSAL_ERROR_MAX_COUNT;
    }

    /* 创建新节点 */
    ListItem_t* item = list_node_create(data, wrapper->node_size);
    if (item == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    /* 插入到position前面 */
    TickType_t pos_value = listGET_LIST_ITEM_VALUE(pos);
    listSET_LIST_ITEM_VALUE(item, (pos_value > 0) ? (pos_value - 1) : 0);
    vListInsert(list, item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_pop_front(osal_list_t handle, void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    if (listLIST_IS_EMPTY(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    /* 获取链表头部项（排除xListEnd） */
    ListItem_t* item = listGET_HEAD_ENTRY(list);
    if (item == listGET_END_MARKER(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    /* 复制数据 */
    if (data != NULL) {
        void* item_data = listGET_LIST_ITEM_OWNER(item);
        if (item_data != NULL) {
            memcpy(data, item_data, wrapper->node_size);
        }
    }

    /* 移除并释放节点 */
    uxListRemove(item);
    list_node_free(item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_pop_back(osal_list_t handle, void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    if (listLIST_IS_EMPTY(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    /* FreeRTOS的链表是循环的，尾部是xListEnd的前一个 */
    const ListItem_t* end_marker = listGET_END_MARKER(list);
    ListItem_t* item = listGET_PREV(end_marker);

    if (item == end_marker) {
        return OSAL_ERROR_NOT_FOUND;
    }

    /* 复制数据 */
    if (data != NULL) {
        void* item_data = listGET_LIST_ITEM_OWNER(item);
        if (item_data != NULL) {
            memcpy(data, item_data, wrapper->node_size);
        }
    }

    /* 移除并释放节点 */
    uxListRemove(item);
    list_node_free(item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_remove(osal_list_t handle, osal_list_node_t* node_param,
                                             void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || node_param == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    ListItem_t* item = (ListItem_t*)node_param;

    /* 复制数据 */
    if (data != NULL) {
        void* item_data = listGET_LIST_ITEM_OWNER(item);
        if (item_data != NULL) {
            memcpy(data, item_data, wrapper->node_size);
        }
    }

    /* 移除并释放节点 */
    uxListRemove(item);
    list_node_free(item);

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_front(osal_list_t handle, osal_list_node_t** node) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    if (listLIST_IS_EMPTY(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    ListItem_t* item = listGET_HEAD_ENTRY(list);
    *node = (osal_list_node_t*)item;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_back(osal_list_t handle, osal_list_node_t** node) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    if (listLIST_IS_EMPTY(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    /* FreeRTOS链表尾部是xListEnd的前一个 */
    const ListItem_t* end_marker = listGET_END_MARKER(list);
    ListItem_t* item = listGET_PREV(end_marker);

    *node = (osal_list_node_t*)item;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_next(osal_list_node_t* node, osal_list_node_t** next) {
#if OSAL_CFG_PARAM_CHECK
    if (node == NULL || next == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    ListItem_t* item = (ListItem_t*)node;
    ListItem_t* next_item = listGET_NEXT(item);

    /* 检查是否到达链表结束标记 */
    List_t* list = listLIST_ITEM_CONTAINER(item);
    if (list != NULL && next_item == listGET_END_MARKER(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    *next = (osal_list_node_t*)next_item;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_prev(osal_list_node_t* node, osal_list_node_t** prev) {
#if OSAL_CFG_PARAM_CHECK
    if (node == NULL || prev == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    ListItem_t* item = (ListItem_t*)node;
    ListItem_t* prev_item = listGET_PREV(item);

    List_t* list = listLIST_ITEM_CONTAINER(item);
    if (list != NULL && prev_item == listGET_END_MARKER(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    *prev = (osal_list_node_t*)prev_item;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_get_data(osal_list_node_t* node, void** data) {
#if OSAL_CFG_PARAM_CHECK
    if (node == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    ListItem_t* item = (ListItem_t*)node;
    *data = listGET_LIST_ITEM_OWNER(item);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_set_data(osal_list_node_t* node, const void* data) {
#if OSAL_CFG_PARAM_CHECK
    if (node == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    /* FreeRTOS的ListItem_t通过pvOwner存储数据，但它是void*指针
     * 我们无法知道原始数据的大小来执行拷贝
     * 这个操作在当前架构下不支持 */
    (void)node;
    (void)data;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_freertos_list_get_count(osal_list_t handle, uint32_t* count) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || count == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;
    *count = listCURRENT_LIST_LENGTH(list);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_is_empty(osal_list_t handle, int* is_empty) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || is_empty == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;
    *is_empty = listLIST_IS_EMPTY(list) ? 1 : 0;
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_clear(osal_list_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    /* 遍历并释放所有节点 */
    ListItem_t* item = listGET_HEAD_ENTRY(list);
    while (item != listGET_END_MARKER(list)) {
        ListItem_t* next = listGET_NEXT(item);
        uxListRemove(item);
        list_node_free(item);
        item = next;
    }

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_reverse(osal_list_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    /* FreeRTOS链表是循环的且按xItemValue排序，直接反转会破坏排序
     * 这个操作需要重建整个链表，实现较为复杂
     */
    (void)list;
    (void)wrapper;

    /* 由于FreeRTOS链表的有序特性，反转操作需要特殊处理
     * 这里返回不支持，或者可以实现为重新调整所有xItemValue */
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_freertos_list_sort(osal_list_t handle, osal_list_compare_cb_t compare) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || compare == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    if (listCURRENT_LIST_LENGTH(list) < 2) {
        return OSAL_OK;
    }

    /* FreeRTOS链表是按xItemValue排序的，需要重新设置所有item的值
     * 收集所有数据，使用用户回调排序，然后重新插入 */
    /* 简化实现：使用冒泡排序交换数据指针 */

    /* 获取所有项到数组 */
    uint32_t count = listCURRENT_LIST_LENGTH(list);
    void** data_array = (void**)pvPortMalloc(sizeof(void*) * count);
    if (data_array == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    ListItem_t* item = listGET_HEAD_ENTRY(list);
    for (uint32_t i = 0; i < count; i++) {
        data_array[i] = listGET_LIST_ITEM_OWNER(item);
        item = listGET_NEXT(item);
    }

    /* 冒泡排序数据数组 */
    for (uint32_t i = 0; i < count - 1; i++) {
        for (uint32_t j = 0; j < count - 1 - i; j++) {
            if (compare(data_array[j], data_array[j + 1]) > 0) {
                void* temp = data_array[j];
                data_array[j] = data_array[j + 1];
                data_array[j + 1] = temp;
            }
        }
    }

    /* 重新分配数据到链表项 */
    item = listGET_HEAD_ENTRY(list);
    for (uint32_t i = 0; i < count; i++) {
        /* 释放旧数据 */
        void* old_data = listGET_LIST_ITEM_OWNER(item);
        if (old_data != NULL) {
            vPortFree(old_data);
        }

        /* 分配并复制新数据 */
        void* new_data = pvPortMalloc(wrapper->node_size);
        if (new_data != NULL) {
            memcpy(new_data, data_array[i], wrapper->node_size);
            listSET_LIST_ITEM_OWNER(item, new_data);
        }

        item = listGET_NEXT(item);
    }

    vPortFree(data_array);
    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_find(osal_list_t handle, const void* data,
                                           osal_list_compare_cb_t compare,
                                           osal_list_node_t** node) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL || compare == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    ListItem_t* item = listGET_HEAD_ENTRY(list);
    while (item != listGET_END_MARKER(list)) {
        void* item_data = listGET_LIST_ITEM_OWNER(item);
        if (item_data != NULL && compare(item_data, data) == 0) {
            *node = (osal_list_node_t*)item;
            return OSAL_OK;
        }
        item = listGET_NEXT(item);
    }

    return OSAL_ERROR_NOT_FOUND;
}

osal_status_t osal_port_freertos_list_traverse(osal_list_t handle, osal_list_traverse_cb_t callback,
                                               void* context) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || callback == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    ListItem_t* item = listGET_HEAD_ENTRY(list);
    while (item != listGET_END_MARKER(list)) {
        ListItem_t* next = listGET_NEXT(item); /* 保存next，因为callback可能删除当前item */
        callback((osal_list_node_t*)item, context);
        item = next;
    }

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_get_info(osal_list_t handle, const char** name,
                                               uint32_t* count, uint32_t* max_nodes) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;

    if (name != NULL) {
        *name = wrapper->name;
    }

    if (count != NULL) {
        *count = listCURRENT_LIST_LENGTH(wrapper->list);
    }

    if (max_nodes != NULL) {
        *max_nodes = wrapper->max_nodes;
    }

    return OSAL_OK;
}

osal_status_t osal_port_freertos_list_at(osal_list_t handle, uint32_t index,
                                         osal_list_node_t** node) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || node == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_freertos_list_t* wrapper = (osal_freertos_list_t*)handle;
    List_t* list = wrapper->list;

    if (index >= listCURRENT_LIST_LENGTH(list)) {
        return OSAL_ERROR_NOT_FOUND;
    }

    ListItem_t* item = listGET_HEAD_ENTRY(list);
    for (uint32_t i = 0; i < index; i++) {
        item = listGET_NEXT(item);
        /* 越界检查 */
        if (item == listGET_END_MARKER(list)) {
            return OSAL_ERROR_NOT_FOUND;
        }
    }

    *node = (osal_list_node_t*)item;
    return OSAL_OK;
}
