/**
 * @file osal_port.h
 * @brief FreeRTOS适配层主头文件
 * @details 统一包含所有组件头文件
 */

#ifndef OSAL_PORT_FREERTOS_H
#define OSAL_PORT_FREERTOS_H

#include "osal.h"
#include "osal_config.h"

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "list.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* ==================== 组件头文件包含 ==================== */
#include "osal_event_port.h"
#include "osal_list_port.h"
#include "osal_memory_port.h"
#include "osal_mutex_port.h"
#include "osal_notify_port.h"
#include "osal_queue_port.h"
#include "osal_scheduler_port.h"
#include "osal_semaphore_port.h"
#include "osal_task_port.h"

/* ==================== 类型映射 ==================== */

/* 任务句柄映射 */
#define OSAL_TASK_HANDLE_TO_FREERTOS(handle) ((TaskHandle_t)(handle))
#define FREERTOS_TASK_HANDLE_TO_OSAL(handle) ((osal_task_t)(handle))

/* 队列句柄映射 */
#define OSAL_QUEUE_HANDLE_TO_FREERTOS(handle) ((QueueHandle_t)(handle))
#define FREERTOS_QUEUE_HANDLE_TO_OSAL(handle) ((osal_queue_t)(handle))

/* 互斥锁句柄映射 */
#define OSAL_MUTEX_HANDLE_TO_FREERTOS(handle) ((SemaphoreHandle_t)(handle))
#define FREERTOS_MUTEX_HANDLE_TO_OSAL(handle) ((osal_mutex_t)(handle))

/* 信号量句柄映射 */
#define OSAL_SEMAPHORE_HANDLE_TO_FREERTOS(handle) ((SemaphoreHandle_t)(handle))
#define FREERTOS_SEMAPHORE_HANDLE_TO_OSAL(handle) ((osal_semaphore_t)(handle))

/* 事件句柄映射 */
#define OSAL_EVENT_HANDLE_TO_FREERTOS(handle) ((EventGroupHandle_t)(handle))
#define FREERTOS_EVENT_HANDLE_TO_OSAL(handle) ((osal_event_t)(handle))

/* ==================== 时间映射 ==================== */

#define OSAL_TICKS_TO_FREERTOS(ticks) ((TickType_t)(ticks))
#define FREERTOS_TICKS_TO_OSAL(ticks) ((osal_tick_t)(ticks))

#define OSAL_WAIT_FREERTOS_FOREVER portMAX_DELAY
#define OSAL_WAIT_FREERTOS_NO_WAIT 0

/* ==================== 优先级映射 ==================== */

#define OSAL_PRIORITY_TO_FREERTOS(prio) (tskIDLE_PRIORITY + (prio))
#define FREERTOS_PRIORITY_TO_OSAL(prio) ((osal_priority_t)((prio) - tskIDLE_PRIORITY))

/* ==================== 适配层接口 ==================== */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FreeRTOS适配层初始化
 * @return OSAL_OK 成功
 */
osal_status_t osal_port_freertos_init(void);

/**
 * @brief FreeRTOS适配层反初始化
 * @return OSAL_OK 成功
 */
osal_status_t osal_port_freertos_deinit(void);

/**
 * @brief 检查当前是否在中断处理模式
 * @return int 非0表示在中断模式，0表示不在中断模式
 */
int inHandlerMode(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_PORT_FREERTOS_H */
