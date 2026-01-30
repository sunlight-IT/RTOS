/**
 * @file osal_list.h
 * @brief OSAL链表组件接口
 * @details 提供双向链表的创建、插入、删除、遍历等功能
 * @note 链表是通用的数据结构，适用于存储和管理动态数据集合
 */

#ifndef OSAL_LIST_H
#define OSAL_LIST_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_LIST_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 链表节点和配置结构体 ==================== */

/**
 * @brief 链表节点结构体
 * @note 这是一个不透明类型，具体定义由适配层实现
 */

/**
 * @brief 链表配置结构体
 */
typedef struct {
    const char* name;   /**< 链表名称 */
    uint32_t max_nodes; /**< 最大节点数量(0表示无限制) */
    uint32_t node_size; /**< 节点数据大小(字节) */
} osal_list_config_t;

/**
 * @brief 链表句柄
 */
typedef void* osal_list_t;

/* ==================== 链表操作接口 ==================== */

/**
 * @brief 创建链表
 * @param config 链表配置结构体指针
 * @param[out] handle 链表句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_list_create(const osal_list_config_t* config, osal_list_t* handle);

/**
 * @brief 删除链表
 * @param handle 链表句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 链表正在被使用
 */
osal_status_t osal_list_delete(osal_list_t handle);

/**
 * @brief 在链表头部插入节点
 * @param handle 链表句柄
 * @param data 数据指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_list_push_front(osal_list_t handle, const void* data);

/**
 * @brief 在链表尾部插入节点
 * @param handle 链表句柄
 * @param data 数据指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_list_push_back(osal_list_t handle, const void* data);

/**
 * @brief 在指定位置后插入节点
 * @param handle 链表句柄
 * @param position 位置节点指针
 * @param data 数据指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_list_insert_after(osal_list_t handle, osal_list_node_t* position,
                                     const void* data);

/**
 * @brief 在指定位置前插入节点
 * @param handle 链表句柄
 * @param position 位置节点指针
 * @param data 数据指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_list_insert_before(osal_list_t handle, osal_list_node_t* position,
                                      const void* data);

/**
 * @brief 从链表头部删除节点
 * @param handle 链表句柄
 * @param[out] data 数据缓冲区指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_EMPTY 链表为空
 */
osal_status_t osal_list_pop_front(osal_list_t handle, void* data);

/**
 * @brief 从链表尾部删除节点
 * @param handle 链表句柄
 * @param[out] data 数据缓冲区指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_EMPTY 链表为空
 */
osal_status_t osal_list_pop_back(osal_list_t handle, void* data);

/**
 * @brief 删除指定节点
 * @param handle 链表句柄
 * @param node 要删除的节点指针
 * @param[out] data 数据缓冲区指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 节点不存在
 */
osal_status_t osal_list_remove(osal_list_t handle, osal_list_node_t* node, void* data);

/**
 * @brief 获取链表头部节点
 * @param handle 链表句柄
 * @param[out] node 节点指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_EMPTY 链表为空
 */
osal_status_t osal_list_front(osal_list_t handle, osal_list_node_t** node);

/**
 * @brief 获取链表尾部节点
 * @param handle 链表句柄
 * @param[out] node 节点指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_EMPTY 链表为空
 */
osal_status_t osal_list_back(osal_list_t handle, osal_list_node_t** node);

/**
 * @brief 获取链表下一个节点
 * @param node 当前节点指针
 * @param[out] next 下一个节点指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 已是最后一个节点
 */
osal_status_t osal_list_next(osal_list_node_t* node, osal_list_node_t** next);

/**
 * @brief 获取链表前一个节点
 * @param node 当前节点指针
 * @param[out] prev 前一个节点指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 已是第一个节点
 */
osal_status_t osal_list_prev(osal_list_node_t* node, osal_list_node_t** prev);

/**
 * @brief 获取节点数据
 * @param node 节点指针
 * @param[out] data 数据指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_get_data(osal_list_node_t* node, void** data);

/**
 * @brief 设置节点数据
 * @param node 节点指针
 * @param data 数据指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_set_data(osal_list_node_t* node, const void* data);

/**
 * @brief 获取链表节点数量
 * @param handle 链表句柄
 * @param[out] count 数量输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_get_count(osal_list_t handle, uint32_t* count);

/**
 * @brief 检查链表是否为空
 * @param handle 链表句柄
 * @param[out] is_empty 是否为空输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_is_empty(osal_list_t handle, int* is_empty);

/**
 * @brief 清空链表
 * @param handle 链表句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_clear(osal_list_t handle);

/**
 * @brief 反转链表
 * @param handle 链表句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_reverse(osal_list_t handle);

/**
 * @brief 排序链表
 * @param handle 链表句柄
 * @param compare 比较函数指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_SUPPORTED 功能不支持
 */
typedef int (*osal_list_compare_cb_t)(const void* data1, const void* data2);
osal_status_t osal_list_sort(osal_list_t handle, osal_list_compare_cb_t compare);

/**
 * @brief 查找节点
 * @param handle 链表句柄
 * @param data 要查找的数据
 * @param compare 比较函数指针
 * @param[out] node 找到的节点指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 未找到
 */
osal_status_t osal_list_find(osal_list_t handle, const void* data, osal_list_compare_cb_t compare,
                             osal_list_node_t** node);

/**
 * @brief 遍历链表
 * @param handle 链表句柄
 * @param callback 遍历回调函数
 * @param context 上下文参数
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
typedef void (*osal_list_traverse_cb_t)(osal_list_node_t* node, void* context);
osal_status_t osal_list_traverse(osal_list_t handle, osal_list_traverse_cb_t callback,
                                 void* context);

/**
 * @brief 获取链表信息
 * @param handle 链表句柄
 * @param[out] name 链表名称输出指针（可为NULL）
 * @param[out] count 节点数量输出指针（可为NULL）
 * @param[out] max_nodes 最大节点数输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_list_get_info(osal_list_t handle, const char** name, uint32_t* count,
                                 uint32_t* max_nodes);

/**
 * @brief 获取链表中第N个节点
 * @param handle 链表句柄
 * @param index 索引(从0开始)
 * @param[out] node 节点指针输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_OUT_OF_RANGE 索引超出范围
 */
osal_status_t osal_list_at(osal_list_t handle, uint32_t index, osal_list_node_t** node);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_LIST_ENABLE */

#endif /* OSAL_LIST_H */
