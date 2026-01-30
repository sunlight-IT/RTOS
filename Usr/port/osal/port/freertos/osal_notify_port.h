/**
 * @file osal_notify_port.h
 * @brief FreeRTOS任务通知组件适配接口
 * @details 定义FreeRTOS到OSAL的任务通知适配接口
 */

#ifndef OSAL_FREERTOS_NOTIFY_PORT_H
#define OSAL_FREERTOS_NOTIFY_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 任务通知适配接口 ==================== */

/**
 * @brief FreeRTOS任务通知发送
 */
osal_status_t osal_port_freertos_task_notify(osal_task_t task, uint32_t value, uint8_t action,
                                             uint32_t* prev_value);

/**
 * @brief FreeRTOS任务通知等待
 */
osal_status_t osal_port_freertos_task_notify_wait(uint32_t clear_bits_entry,
                                                  uint32_t clear_bits_exit, uint32_t* value,
                                                  osal_tick_t timeout);

/**
 * @brief FreeRTOS任务通知清除
 */
osal_status_t osal_port_freertos_task_notify_clear(osal_task_t task, uint32_t bits_to_clear,
                                                   uint32_t* prev_value);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_NOTIFY_PORT_H */
