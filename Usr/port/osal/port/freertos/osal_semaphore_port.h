/**
 * @file osal_semaphore_port.h
 * @brief FreeRTOS信号量组件适配接口
 * @details 定义FreeRTOS到OSAL的信号量适配接口
 */

#ifndef OSAL_FREERTOS_SEMAPHORE_PORT_H
#define OSAL_FREERTOS_SEMAPHORE_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 信号量适配接口 ==================== */

/**
 * @brief FreeRTOS信号量创建
 */
osal_status_t osal_port_freertos_semaphore_create(const osal_semaphore_config_t* config,
                                                  osal_semaphore_t* handle);

/**
 * @brief FreeRTOS信号量删除
 */
osal_status_t osal_port_freertos_semaphore_delete(osal_semaphore_t handle);

/**
 * @brief FreeRTOS信号量获取
 */
osal_status_t osal_port_freertos_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout);

/**
 * @brief FreeRTOS信号量释放
 */
osal_status_t osal_port_freertos_semaphore_release(osal_semaphore_t handle);

/**
 * @brief 获取信号量计数值
 */
osal_status_t osal_port_freertos_semaphore_get_count(osal_semaphore_t handle, uint32_t* count);

/**
 * @brief 设置信号量计数值
 */
osal_status_t osal_port_freertos_semaphore_set_count(osal_semaphore_t handle, uint32_t count);

/**
 * @brief 获取信号量信息
 */
osal_status_t osal_port_freertos_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                                    osal_semaphore_type_t* type,
                                                    uint32_t* max_count);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_SEMAPHORE_PORT_H */
