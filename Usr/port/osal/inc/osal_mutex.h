/**
 * @file osal_mutex.h
 * @brief OSAL互斥锁组件接口
 * @details 提供互斥锁创建、获取、释放等功能，支持优先级继承
 */

#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_MUTEX_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 互斥锁配置结构体 ==================== */

/**
 * @brief 互斥锁配置结构体
 */
typedef struct {
    const char* name; /**< 互斥锁名称 */
    uint8_t inherit;  /**< 是否启用优先级继承（1启用，0不启用） */
} osal_mutex_config_t;

/* ==================== 互斥锁操作接口 ==================== */

/**
 * @brief 创建互斥锁
 * @param config 互斥锁配置结构体指针
 * @param[out] handle 互斥锁句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_mutex_create(const osal_mutex_config_t* config, osal_mutex_t* handle);

/**
 * @brief 删除互斥锁
 * @param handle 互斥锁句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 互斥锁正在被使用
 */
osal_status_t osal_mutex_delete(osal_mutex_t handle);

/**
 * @brief 获取互斥锁
 * @param handle 互斥锁句柄
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout);

/**
 * @brief 释放互斥锁
 * @param handle 互斥锁句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 不是互斥锁的持有者
 */
osal_status_t osal_mutex_release(osal_mutex_t handle);

/**
 * @brief 尝试获取互斥锁（非阻塞）
 * @param handle 互斥锁句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_BUSY 互斥锁已被占用
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_mutex_try_acquire(osal_mutex_t handle);

/**
 * @brief 获取互斥锁的持有者
 * @param handle 互斥锁句柄
 * @param[out] owner 持有者任务句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner);

/**
 * @brief 获取互斥锁信息
 * @param handle 互斥锁句柄
 * @param[out] name 互斥锁名称输出指针（可为NULL）
 * @param[out] inherit 优先级继承标志输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_mutex_get_info(osal_mutex_t handle, const char** name, uint8_t* inherit);

/**
 * @brief 递归获取互斥锁
 * @param handle 互斥锁句柄
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_mutex_recursive_acquire(osal_mutex_t handle, osal_tick_t timeout);

/**
 * @brief 递归释放互斥锁
 * @param handle 互斥锁句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_mutex_recursive_release(osal_mutex_t handle);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_MUTEX_ENABLE */

#endif /* OSAL_MUTEX_H */
