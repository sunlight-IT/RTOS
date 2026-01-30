/**
 * @file osal_event_port.h
 * @brief FreeRTOS事件组件适配接口
 * @details 定义FreeRTOS到OSAL的事件适配接口
 */

#ifndef OSAL_FREERTOS_EVENT_PORT_H
#define OSAL_FREERTOS_EVENT_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 事件适配接口 ==================== */

/**
 * @brief FreeRTOS事件创建
 */
osal_status_t osal_port_freertos_event_create(const osal_event_config_t* config,
                                              osal_event_t* handle);

/**
 * @brief FreeRTOS事件删除
 */
osal_status_t osal_port_freertos_event_delete(osal_event_t handle);

/**
 * @brief FreeRTOS事件等待
 */
osal_status_t osal_port_freertos_event_wait(osal_event_t handle, osal_event_flags_t wait_flags,
                                            uint8_t option, osal_event_flags_t* actual_flags,
                                            osal_tick_t timeout);

/**
 * @brief FreeRTOS事件设置
 */
osal_status_t osal_port_freertos_event_set(osal_event_t handle, osal_event_flags_t flags);

/**
 * @brief FreeRTOS事件清除
 */
osal_status_t osal_port_freertos_event_clear(osal_event_t handle, osal_event_flags_t flags);

/**
 * @brief FreeRTOS事件获取
 */
osal_status_t osal_port_freertos_event_get(osal_event_t handle, osal_event_flags_t* flags);

/**
 * @brief FreeRTOS事件同步
 */
osal_status_t osal_port_freertos_event_sync(osal_event_t handle, osal_event_flags_t flags);

/**
 * @brief 获取事件信息
 */
osal_status_t osal_port_freertos_event_get_info(osal_event_t handle, const char** name);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_EVENT_PORT_H */
