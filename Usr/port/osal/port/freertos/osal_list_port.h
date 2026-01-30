/**
 * @file osal_list_port.h
 * @brief FreeRTOS链表组件适配接口
 * @details 定义FreeRTOS到OSAL的链表适配接口
 * @note 使用FreeRTOS原生链表组件 (List_t/ListItem_t)
 */

#ifndef OSAL_FREERTOS_LIST_PORT_H
#define OSAL_FREERTOS_LIST_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 链表内部数据结构 ==================== */

/**
 * @brief 链表节点结构体（FreeRTOS适配层内部使用）
 * @note 使用FreeRTOS原生ListItem_t，通过pvOwner存储数据指针
 * ListItem_t已定义在FreeRTOS的list.h中
 */
typedef ListItem_t osal_freertos_list_node_t;

/**
 * @brief 链表结构体（FreeRTOS适配层内部使用）
 * @note 使用FreeRTOS原生List_t作为底层实现
 * List_t已定义在FreeRTOS的list.h中
 */
typedef struct {
    List_t* list;       /**< FreeRTOS原生链表指针 */
    const char* name;   /**< 链表名称 */
    uint32_t max_nodes; /**< 最大节点数(0表示无限制) */
    uint32_t node_size; /**< 节点数据大小 */
} osal_freertos_list_t;

/* ==================== 链表适配接口 ==================== */

/**
 * @brief FreeRTOS链表创建
 */
osal_status_t osal_port_freertos_list_create(const osal_list_config_t* config, osal_list_t* handle);

/**
 * @brief FreeRTOS链表删除
 */
osal_status_t osal_port_freertos_list_delete(osal_list_t handle);

/**
 * @brief FreeRTOS链表前端插入
 */
osal_status_t osal_port_freertos_list_push_front(osal_list_t handle, const void* data);

/**
 * @brief FreeRTOS链表后端插入
 */
osal_status_t osal_port_freertos_list_push_back(osal_list_t handle, const void* data);

/**
 * @brief FreeRTOS链表节点后插入
 */
osal_status_t osal_port_freertos_list_insert_after(osal_list_t handle, osal_list_node_t* position,
                                                   const void* data);

/**
 * @brief FreeRTOS链表节点前插入
 */
osal_status_t osal_port_freertos_list_insert_before(osal_list_t handle, osal_list_node_t* position,
                                                    const void* data);

/**
 * @brief FreeRTOS链表前端弹出
 */
osal_status_t osal_port_freertos_list_pop_front(osal_list_t handle, void* data);

/**
 * @brief FreeRTOS链表后端弹出
 */
osal_status_t osal_port_freertos_list_pop_back(osal_list_t handle, void* data);

/**
 * @brief FreeRTOS链表移除节点
 */
osal_status_t osal_port_freertos_list_remove(osal_list_t handle, osal_list_node_t* node,
                                             void* data);

/**
 * @brief FreeRTOS链表获取头节点
 */
osal_status_t osal_port_freertos_list_front(osal_list_t handle, osal_list_node_t** node);

/**
 * @brief FreeRTOS链表获取尾节点
 */
osal_status_t osal_port_freertos_list_back(osal_list_t handle, osal_list_node_t** node);

/**
 * @brief FreeRTOS链表获取下一个节点
 */
osal_status_t osal_port_freertos_list_next(osal_list_node_t* node, osal_list_node_t** next);

/**
 * @brief FreeRTOS链表获取上一个节点
 */
osal_status_t osal_port_freertos_list_prev(osal_list_node_t* node, osal_list_node_t** prev);

/**
 * @brief FreeRTOS链表获取节点数据
 */
osal_status_t osal_port_freertos_list_get_data(osal_list_node_t* node, void** data);

/**
 * @brief FreeRTOS链表设置节点数据
 */
osal_status_t osal_port_freertos_list_set_data(osal_list_node_t* node, const void* data);

/**
 * @brief FreeRTOS链表获取节点数量
 */
osal_status_t osal_port_freertos_list_get_count(osal_list_t handle, uint32_t* count);

/**
 * @brief FreeRTOS链表检查是否为空
 */
osal_status_t osal_port_freertos_list_is_empty(osal_list_t handle, int* is_empty);

/**
 * @brief FreeRTOS链表清空
 */
osal_status_t osal_port_freertos_list_clear(osal_list_t handle);

/**
 * @brief FreeRTOS链表反转
 */
osal_status_t osal_port_freertos_list_reverse(osal_list_t handle);

/**
 * @brief FreeRTOS链表排序
 */
osal_status_t osal_port_freertos_list_sort(osal_list_t handle, osal_list_compare_cb_t compare);

/**
 * @brief FreeRTOS链表查找
 */
osal_status_t osal_port_freertos_list_find(osal_list_t handle, const void* data,
                                           osal_list_compare_cb_t compare, osal_list_node_t** node);

/**
 * @brief FreeRTOS链表遍历
 */
osal_status_t osal_port_freertos_list_traverse(osal_list_t handle, osal_list_traverse_cb_t callback,
                                               void* context);

/**
 * @brief FreeRTOS链表获取信息
 */
osal_status_t osal_port_freertos_list_get_info(osal_list_t handle, const char** name,
                                               uint32_t* count, uint32_t* max_nodes);

/**
 * @brief FreeRTOS链表按索引获取节点
 */
osal_status_t osal_port_freertos_list_at(osal_list_t handle, uint32_t index,
                                         osal_list_node_t** node);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_LIST_PORT_H */
