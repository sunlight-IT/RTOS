/**
 * @file osal_mutex_port.h
 * @brief FreeRTOS互斥锁组件适配接口
 * @details 定义FreeRTOS到OSAL的互斥锁适配接口
 */

#ifndef OSAL_FREERTOS_MUTEX_PORT_H
#define OSAL_FREERTOS_MUTEX_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 互斥锁适配接口 ==================== */

/**
 * @brief FreeRTOS互斥锁创建
 */
osal_status_t osal_port_freertos_mutex_create(const osal_mutex_config_t* config,
                                              osal_mutex_t* handle);

/**
 * @brief FreeRTOS互斥锁删除
 */
osal_status_t osal_port_freertos_mutex_delete(osal_mutex_t handle);

/**
 * @brief FreeRTOS互斥锁获取
 */
osal_status_t osal_port_freertos_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout);

/**
 * @brief FreeRTOS互斥锁释放
 */
osal_status_t osal_port_freertos_mutex_release(osal_mutex_t handle);

/**
 * @brief 获取互斥锁持有者
 */
osal_status_t osal_port_freertos_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner);

/**
 * @brief 获取互斥锁信息
 */
osal_status_t osal_port_freertos_mutex_get_info(osal_mutex_t handle, const char** name,
                                                uint8_t* inherit);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_MUTEX_PORT_H */
