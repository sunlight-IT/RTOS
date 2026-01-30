/**
 * @file osal_memory_port.h
 * @brief FreeRTOS内存管理组件适配接口
 * @details 定义FreeRTOS到OSAL的内存管理适配接口
 */

#ifndef OSAL_FREERTOS_MEMORY_PORT_H
#define OSAL_FREERTOS_MEMORY_PORT_H

#include "osal_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 内存管理适配接口 ==================== */

/**
 * @brief FreeRTOS内存分配
 */
void* osal_port_freertos_memory_alloc(uint32_t size);

/**
 * @brief FreeRTOS内存释放
 */
osal_status_t osal_port_freertos_memory_free(void* ptr);

/**
 * @brief FreeRTOS内存重新分配
 */
void* osal_port_freertos_memory_realloc(void* ptr, uint32_t size);

/**
 * @brief FreeRTOS内存分配并清零
 */
void* osal_port_freertos_memory_calloc(uint32_t num, uint32_t size);

/**
 * @brief 获取空闲内存大小
 */
uint32_t osal_port_freertos_memory_get_free_size(void);

/**
 * @brief 获取历史最小空闲内存大小
 */
uint32_t osal_port_freertos_memory_get_minimum_free_size(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FREERTOS_MEMORY_PORT_H */
