/**
 * @file osal_task.h
 * @brief OSAL任务组件接口
 * @details 提供任务创建、删除、挂起、恢复、优先级设置等功能
 */

#ifndef OSAL_TASK_H
#define OSAL_TASK_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_TASK_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 任务操作接口 ==================== */

/**
 * @brief 创建任务
 * @param config 任务配置结构体指针
 * @param[out] handle 任务句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 *         OSAL_ERROR_MAX_COUNT 任务数量已达上限
 */
osal_status_t osal_task_create(const osal_task_config_t* config, osal_task_t* handle);

/**
 * @brief 删除任务
 * @param handle 任务句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 */
osal_status_t osal_task_delete(osal_task_t handle);

/**
 * @brief 挂起任务
 * @param handle 任务句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 *         OSAL_ERROR_ISR 在ISR中调用
 */
osal_status_t osal_task_suspend(osal_task_t handle);

/**
 * @brief 恢复任务
 * @param handle 任务句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 *         OSAL_ERROR_ISR 在ISR中调用
 */
osal_status_t osal_task_resume(osal_task_t handle);

/**
 * @brief 获取当前任务句柄
 * @return 当前任务句柄，NULL表示失败或无任务
 */
osal_task_t osal_task_get_current(void);

/**
 * @brief 让出CPU
 * @details 当前任务主动让出CPU，让同优先级或更高优先级任务运行
 * @return OSAL_OK 成功
 */
osal_status_t osal_task_yield(void);

/**
 * @brief 任务延时
 * @param ticks 延时的tick数
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_task_delay(osal_tick_t ticks);

/**
 * @brief 任务延时到指定时间（绝对时间）
 * @param ticks 指定的tick时间点
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks);

/**
 * @brief 获取任务优先级
 * @param handle 任务句柄（NULL表示当前任务）
 * @param[out] priority 优先级输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 */
osal_status_t osal_task_get_priority(osal_task_t handle, osal_priority_t* priority);

/**
 * @brief 设置任务优先级
 * @param handle 任务句柄（NULL表示当前任务）
 * @param priority 新的优先级
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 */
osal_status_t osal_task_set_priority(osal_task_t handle, osal_priority_t priority);

/**
 * @brief 获取任务状态
 * @param handle 任务句柄
 * @param[out] state 状态输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 */
osal_status_t osal_task_get_state(osal_task_t handle, osal_task_state_t* state);

/**
 * @brief 获取任务信息
 * @param handle 任务句柄
 * @param[out] name 任务名称输出指针（可为NULL）
 * @param[out] stack_size 栈大小输出指针（可为NULL）
 * @param[out] stack_free 剩余栈空间输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NOT_FOUND 任务不存在
 */
osal_status_t osal_task_get_info(osal_task_t handle, const char** name, uint32_t* stack_size,
                                 uint32_t* stack_free);

/**
 * @brief 获取系统tick数
 * @return 当前系统tick数
 */
osal_tick_t osal_task_get_tick_count(void);

/**
 * @brief 检查是否在ISR上下文中
 * @return 1 在ISR中，0 不在ISR中
 */
int osal_task_is_in_isr(void);

/**
 * @brief 任务自旋等待
 * @param handle 任务句柄（NULL表示当前任务）
 * @return OSAL_OK 成功
 */
osal_status_t osal_task_suspended(osal_task_t handle);

/**
 * @brief 获取任务数量
 * @return 当前任务数量
 */
uint32_t osal_task_get_count(void);

/**
 * @brief 获取所有任务列表
 * @param tasks 任务句柄数组
 * @param max_count 数组最大容量
 * @return 实际任务数量
 */
uint32_t osal_task_get_list(osal_task_t* tasks, uint32_t max_count);

/* ==================== 任务Hook函数 ==================== */

/**
 * @brief 任务创建Hook函数
 */
OSAL_WEAK void osal_task_create_hook(osal_task_t handle);

/**
 * @brief 任务删除Hook函数
 */
OSAL_WEAK void osal_task_delete_hook(osal_task_t handle);

/**
 * @brief 任务切换Hook函数
 */
OSAL_WEAK void osal_task_switch_hook(void);

/**
 * @brief 空闲任务Hook函数
 */
OSAL_WEAK void osal_idle_task_hook(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_TASK_ENABLE */

#endif /* OSAL_TASK_H */
