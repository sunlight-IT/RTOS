/**
 * @file osal_task_port.h
 * @brief FreeRTOS任务组件适配接口
 * @details 定义FreeRTOS到OSAL的任务适配接口
 */

#ifndef OSAL_FREERTOS_TASK_PORT_H
#define OSAL_FREERTOS_TASK_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 任务适配接口 ==================== */

/**
 * @brief FreeRTOS任务创建
 */
osal_status_t osal_port_freertos_task_create(const osal_task_config_t* config, osal_task_t* handle);

/**
 * @brief FreeRTOS任务删除
 */
osal_status_t osal_port_freertos_task_delete(osal_task_t handle);

/**
 * @brief FreeRTOS任务挂起
 */
osal_status_t osal_port_freertos_task_suspend(osal_task_t handle);

/**
 * @brief FreeRTOS任务恢复
 */
osal_status_t osal_port_freertos_task_resume(osal_task_t handle);

/**
 * @brief 获取当前任务句柄
 */
osal_task_t osal_port_freertos_task_get_current(void);

/**
 * @brief 任务让出CPU
 */
osal_status_t osal_port_freertos_task_yield(void);

/**
 * @brief 任务延时
 */
osal_status_t osal_port_freertos_task_delay(osal_tick_t ticks);

/**
 * @brief 任务绝对延时
 */
osal_status_t osal_port_freertos_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks);

/**
 * @brief 获取任务优先级
 */
osal_status_t osal_port_freertos_task_get_priority(osal_task_t handle, osal_priority_t* priority);

/**
 * @brief 设置任务优先级
 */
osal_status_t osal_port_freertos_task_set_priority(osal_task_t handle, osal_priority_t priority);

/**
 * @brief 获取任务状态
 */
osal_status_t osal_port_freertos_task_get_state(osal_task_t handle, osal_task_state_t* state);

/**
 * @brief 获取任务信息
 */
osal_status_t osal_port_freertos_task_get_info(osal_task_t handle, const char** name,
                                               uint32_t* stack_size, uint32_t* stack_free);

/**
 * @brief 获取系统节拍计数
 */
osal_tick_t osal_port_freertos_task_get_tick_count(void);

/**
 * @brief 检查是否在ISR中
 */
int osal_port_freertos_task_is_in_isr(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_TASK_PORT_H */
