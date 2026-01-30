/**
 * @file osal_scheduler_port.h
 * @brief FreeRTOS调度器控制组件适配接口
 * @details 定义FreeRTOS到OSAL的调度器控制适配接口
 */

#ifndef OSAL_FREERTOS_SCHEDULER_PORT_H
#define OSAL_FREERTOS_SCHEDULER_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 调度器控制适配接口 ==================== */

/**
 * @brief FreeRTOS调度器挂起
 */
osal_status_t osal_port_freertos_scheduler_suspend(void);

/**
 * @brief FreeRTOS调度器恢复
 */
osal_status_t osal_port_freertos_scheduler_resume(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_SCHEDULER_PORT_H */
