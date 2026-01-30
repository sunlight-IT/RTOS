/**
 * @file osal_queue_port.h
 * @brief FreeRTOS队列组件适配接口
 * @details 定义FreeRTOS到OSAL的队列适配接口
 */

#ifndef OSAL_FREERTOS_QUEUE_PORT_H
#define OSAL_FREERTOS_QUEUE_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 队列适配接口 ==================== */

/**
 * @brief FreeRTOS队列创建
 */
osal_status_t osal_port_freertos_queue_create(const osal_queue_config_t* config,
                                              osal_queue_t* handle);

/**
 * @brief FreeRTOS队列删除
 */
osal_status_t osal_port_freertos_queue_delete(osal_queue_t handle);

/**
 * @brief FreeRTOS队列发送
 */
osal_status_t osal_port_freertos_queue_send(osal_queue_t handle, const void* data,
                                            osal_tick_t timeout);

/**
 * @brief FreeRTOS队列前端发送
 */
osal_status_t osal_port_freertos_queue_send_front(osal_queue_t handle, const void* data,
                                                  osal_tick_t timeout);

/**
 * @brief FreeRTOS队列接收
 */
osal_status_t osal_port_freertos_queue_receive(osal_queue_t handle, void* data,
                                               osal_tick_t timeout);

/**
 * @brief FreeRTOS队列重置
 */
osal_status_t osal_port_freertos_queue_reset(osal_queue_t handle);

/**
 * @brief 获取队列消息数量
 */
osal_status_t osal_port_freertos_queue_get_count(osal_queue_t handle, uint32_t* count);

/**
 * @brief 获取队列剩余空间
 */
osal_status_t osal_port_freertos_queue_get_space(osal_queue_t handle, uint32_t* space);

/**
 * @brief 检查队列是否为空
 */
osal_status_t osal_port_freertos_queue_is_empty(osal_queue_t handle, int* is_empty);

/**
 * @brief 检查队列是否已满
 */
osal_status_t osal_port_freertos_queue_is_full(osal_queue_t handle, int* is_full);

/**
 * @brief 获取队列信息
 */
osal_status_t osal_port_freertos_queue_get_info(osal_queue_t handle, const char** name,
                                                uint32_t* max_items, uint32_t* item_size);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_QUEUE_PORT_H */
