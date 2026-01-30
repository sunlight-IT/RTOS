/**
 * @file osal_task_notify.h
 * @brief OSAL任务通知组件接口
 * @details 提供任务通知功能，用于任务间轻量级同步和通信
 * @note 任务通知比信号量和事件组更高效，是FreeRTOS特有的轻量级同步机制
 */

#ifndef OSAL_TASK_NOTIFY_H
#define OSAL_TASK_NOTIFY_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_TASK_NOTIFY_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 任务通知配置结构体 ==================== */

/**
 * @brief 任务通知配置结构体
 */
typedef struct {
    const char* name; /**< 任务通知名称 */
} osal_task_notify_config_t;

/* ==================== 任务通知操作接口 ==================== */

/**
 * @brief 发送任务通知
 * @param task 目标任务句柄（NULL表示当前任务）
 * @param value 通知值
 * @param action 通知动作（OSAL_TASK_NOTIFY_xxx宏定义）
 * @param[out] prev_value 发送前的通知值（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_task_notify(osal_task_t task, uint32_t value, uint8_t action,
                               uint32_t* prev_value);

/**
 * @brief 等待任务通知
 * @param[out] value 接收到的通知值（可为NULL）
 * @param timeout 超时时间(ms)
 * @param clear_on_exit 退出时是否清除通知
 * @return OSAL_OK 成功
 *         OSAL_ERROR_TIMEOUT 超时
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_task_notify_wait(uint32_t clear_bits_entry, uint32_t clear_bits_exit,
                                    uint32_t* value, osal_tick_t timeout);

/**
 * @brief 清除任务通知
 * @param task 目标任务句柄（NULL表示当前任务）
 * @param bits_to_clear 要清除的位
 * @param[out] prev_value 清除前的通知值（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_task_notify_clear(osal_task_t task, uint32_t bits_to_clear,
                                     uint32_t* prev_value);

/* ==================== 任务通知动作宏定义 ==================== */

/**
 * @brief 任务通知动作枚举
 */
#define OSAL_TASK_NOTIFY_NO_ACTION 0x00                   /**< 不执行任何动作（仅设置值） */
#define OSAL_TASK_NOTIFY_SET_BITS 0x01                    /**< 设置位 */
#define OSAL_TASK_NOTIFY_INCREMENT 0x02                   /**< 递增计数 */
#define OSAL_TASK_NOTIFY_SET_VALUE_WITHOUT_OVERWRITE 0x03 /**< 设置值（不覆盖） */
#define OSAL_TASK_NOTIFY_SET_VALUE 0x04                   /**< 设置值（覆盖） */

/* ==================== 任务通知状态宏定义 ==================== */

/**
 * @brief 任务通知状态枚举
 */
#define OSAL_TASK_NOTIFY_STATE_NOT_PENDING 0x00 /**< 通知未挂起 */
#define OSAL_TASK_NOTIFY_STATE_PENDING 0x01     /**< 通知已挂起 */

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_TASK_NOTIFY_ENABLE */

#endif /* OSAL_TASK_NOTIFY_H */
