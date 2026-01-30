/**
 * @file osal_semaphore.h
 * @brief OSAL信号量组件接口
 * @details 提供信号量创建、获取、释放等功能，支持二值信号量和计数信号量
 */

#ifndef OSAL_SEMAPHORE_H
#define OSAL_SEMAPHORE_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_SEMAPHORE_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 信号量类型 ==================== */

/**
 * @brief 信号量类型枚举
 */
typedef enum {
    OSAL_SEMAPHORE_BINARY = 0,  /**< 二值信号量 */
    OSAL_SEMAPHORE_COUNTING = 1 /**< 计数信号量 */
} osal_semaphore_type_t;

/* ==================== 信号量配置结构体 ==================== */

/**
 * @brief 信号量配置结构体
 */
typedef struct {
    const char* name;           /**< 信号量名称 */
    osal_semaphore_type_t type; /**< 信号量类型 */
    uint32_t max_count;         /**< 最大计数值（计数信号量使用） */
    uint32_t init_count;        /**< 初始计数值 */
} osal_semaphore_config_t;

/* ==================== 信号量操作接口 ==================== */

/**
 * @brief 创建信号量
 * @param config 信号量配置结构体指针
 * @param[out] handle 信号量句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_semaphore_create(const osal_semaphore_config_t* config,
                                    osal_semaphore_t* handle);

/**
 * @brief 删除信号量
 * @param handle 信号量句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 信号量正在被使用
 */
osal_status_t osal_semaphore_delete(osal_semaphore_t handle);

/**
 * @brief 获取信号量
 * @param handle 信号量句柄
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout);

/**
 * @brief 释放信号量
 * @param handle 信号量句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_MAX_COUNT 计数已达最大值
 */
osal_status_t osal_semaphore_release(osal_semaphore_t handle);

/**
 * @brief 获取信号量当前计数值
 * @param handle 信号量句柄
 * @param[out] count 计数值输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_semaphore_get_count(osal_semaphore_t handle, uint32_t* count);

/**
 * @brief 设置信号量计数值
 * @param handle 信号量句柄
 * @param count 新的计数值
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_semaphore_set_count(osal_semaphore_t handle, uint32_t count);

/**
 * @brief 获取信号量信息
 * @param handle 信号量句柄
 * @param[out] name 信号量名称输出指针（可为NULL）
 * @param[out] type 信号量类型输出指针（可为NULL）
 * @param[out] max_count 最大计数值输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                      osal_semaphore_type_t* type, uint32_t* max_count);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_SEMAPHORE_ENABLE */

#endif /* OSAL_SEMAPHORE_H */
