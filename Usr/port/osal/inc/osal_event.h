/**
 * @file osal_event.h
 * @brief OSAL事件组件接口
 * @details 提供事件创建、触发、等待等功能，支持多事件标志
 */

#ifndef OSAL_EVENT_H
#define OSAL_EVENT_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_EVENT_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 事件配置结构体 ==================== */

/**
 * @brief 事件配置结构体
 */
typedef struct {
    const char* name; /**< 事件组名称 */
} osal_event_config_t;

/* ==================== 事件操作接口 ==================== */

/**
 * @brief 创建事件组
 * @param config 事件配置结构体指针
 * @param[out] handle 事件句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_event_create(const osal_event_config_t* config, osal_event_t* handle);

/**
 * @brief 删除事件组
 * @param handle 事件句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 事件正在被使用
 */
osal_status_t osal_event_delete(osal_event_t handle);

/**
 * @brief 等待事件标志
 * @param handle 事件句柄
 * @param wait_flags 等待的事件标志位
 * @param option 等待选项（OSAL_EVENT_WAIT_ANY/ALL/CLEAR）
 * @param[out] actual_flags 实际触发的事件标志位输出指针（可为NULL）
 * @param timeout 超时时间(ms)
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_TIMEOUT 超时
 */
osal_status_t osal_event_wait(osal_event_t handle, osal_event_flags_t wait_flags, uint8_t option,
                              osal_event_flags_t* actual_flags, osal_tick_t timeout);

/**
 * @brief 设置事件标志
 * @param handle 事件句柄
 * @param flags 要设置的事件标志位
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_event_set(osal_event_t handle, osal_event_flags_t flags);

/**
 * @brief 清除事件标志
 * @param handle 事件句柄
 * @param flags 要清除的事件标志位
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_event_clear(osal_event_t handle, osal_event_flags_t flags);

/**
 * @brief 获取当前事件标志
 * @param handle 事件句柄
 * @param[out] flags 当前事件标志位输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_event_get(osal_event_t handle, osal_event_flags_t* flags);

/**
 * @brief 同步设置事件并唤醒等待任务（原子操作）
 * @param handle 事件句柄
 * @param flags 要设置的事件标志位
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_event_sync(osal_event_t handle, osal_event_flags_t flags);

/**
 * @brief 获取事件组信息
 * @param handle 事件句柄
 * @param[out] name 事件组名称输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_event_get_info(osal_event_t handle, const char** name);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_EVENT_ENABLE */

#endif /* OSAL_EVENT_H */
