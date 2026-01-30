/**
 * @file osal_queue.h
 * @brief OSAL队列组件接口
 * @details 提供队列创建、发送、接收、删除等功能
 */

#ifndef OSAL_QUEUE_H
#define OSAL_QUEUE_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_QUEUE_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 队列配置结构体 ==================== */

/**
 * @brief 队列配置结构体
 */
typedef struct {
    const char* name;   /**< 队列名称 */
    uint32_t max_items; /**< 最大项目数 */
    uint32_t item_size; /**< 每个项目大小(字节) */
} osal_queue_config_t;

/* ==================== 队列操作接口 ==================== */

/**
 * @brief 创建队列
 * @param config 队列配置结构体指针
 * @param[out] handle 队列句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_queue_create(const osal_queue_config_t* config, osal_queue_t* handle);

/**
 * @brief 删除队列
 * @param handle 队列句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 队列正在被使用
 */
osal_status_t osal_queue_delete(osal_queue_t handle);

/**
 * @brief 向队列发送数据（后端插入）
 * @param handle 队列句柄
 * @param data 数据指针
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_queue_send(osal_queue_t handle, const void* data, osal_tick_t timeout);

/**
 * @brief 向队列发送数据（前端插入）
 * @param handle 队列句柄
 * @param data 数据指针
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_queue_send_front(osal_queue_t handle, const void* data, osal_tick_t timeout);

/**
 * @brief 从队列接收数据
 * @param handle 队列句柄
 * @param[out] data 数据缓冲区指针
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_queue_receive(osal_queue_t handle, void* data, osal_tick_t timeout);

/**
 * @brief 清空队列
 * @param handle 队列句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_queue_reset(osal_queue_t handle);

/**
 * @brief 获取队列中项目数量
 * @param handle 队列句柄
 * @param[out] count 数量输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_queue_get_count(osal_queue_t handle, uint32_t* count);

/**
 * @brief 获取队列空闲空间
 * @param handle 队列句柄
 * @param[out] space 空闲空间输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_queue_get_space(osal_queue_t handle, uint32_t* space);

/**
 * @brief 检查队列是否为空
 * @param handle 队列句柄
 * @param[out] is_empty 是否为空输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_queue_is_empty(osal_queue_t handle, int* is_empty);

/**
 * @brief 检查队列是否已满
 * @param handle 队列句柄
 * @param[out] is_full 是否已满输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_queue_is_full(osal_queue_t handle, int* is_full);

/**
 * @brief 获取队列信息
 * @param handle 队列句柄
 * @param[out] name 队列名称输出指针（可为NULL）
 * @param[out] max_items 最大项目数输出指针（可为NULL）
 * @param[out] item_size 项目大小输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_queue_get_info(osal_queue_t handle, const char** name, uint32_t* max_items,
                                  uint32_t* item_size);

/**
 * @brief 遍历队列（不删除）
 * @param handle 队列句柄
 * @param callback 遍历回调函数
 * @param context 上下文参数
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
typedef void (*osal_queue_traverse_cb_t)(const void* data, void* context);
osal_status_t osal_queue_traverse(osal_queue_t handle, osal_queue_traverse_cb_t callback,
                                  void* context);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_QUEUE_ENABLE */

#endif /* OSAL_QUEUE_H */
