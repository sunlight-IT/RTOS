/**
 * @file osal_port.h
 * @brief FreeRTOS适配层头文件
 * @details 定义FreeRTOS到OSAL的适配接口
 */

#ifndef OSAL_PORT_FREERTOS_H
#define OSAL_PORT_FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osal.h"
#include "osal_config.h"

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

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

/* ==================== 任务适配接口 ==================== */

osal_status_t osal_port_freertos_task_create(const osal_task_config_t* config, osal_task_t* handle);
osal_status_t osal_port_freertos_task_delete(osal_task_t handle);
osal_status_t osal_port_freertos_task_suspend(osal_task_t handle);
osal_status_t osal_port_freertos_task_resume(osal_task_t handle);
osal_task_t osal_port_freertos_task_get_current(void);
osal_status_t osal_port_freertos_task_yield(void);
osal_status_t osal_port_freertos_task_delay(osal_tick_t ticks);
osal_status_t osal_port_freertos_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks);
osal_status_t osal_port_freertos_task_get_priority(osal_task_t handle, osal_priority_t* priority);
osal_status_t osal_port_freertos_task_set_priority(osal_task_t handle, osal_priority_t priority);
osal_status_t osal_port_freertos_task_get_state(osal_task_t handle, osal_task_state_t* state);
osal_status_t osal_port_freertos_task_get_info(osal_task_t handle, const char** name,
                                               uint32_t* stack_size, uint32_t* stack_free);
osal_tick_t osal_port_freertos_task_get_tick_count(void);
int osal_port_freertos_task_is_in_isr(void);

/* ==================== 队列适配接口 ==================== */

osal_status_t osal_port_freertos_queue_create(const osal_queue_config_t* config,
                                              osal_queue_t* handle);
osal_status_t osal_port_freertos_queue_delete(osal_queue_t handle);
osal_status_t osal_port_freertos_queue_send(osal_queue_t handle, const void* data,
                                            osal_tick_t timeout);
osal_status_t osal_port_freertos_queue_send_front(osal_queue_t handle, const void* data,
                                                  osal_tick_t timeout);
osal_status_t osal_port_freertos_queue_receive(osal_queue_t handle, void* data,
                                               osal_tick_t timeout);
osal_status_t osal_port_freertos_queue_send_from_isr(osal_queue_t handle, const void* data,
                                                     int* higher_pri_task_woken);
osal_status_t osal_port_freertos_queue_receive_from_isr(osal_queue_t handle, void* data,
                                                        int* higher_pri_task_woken);
osal_status_t osal_port_freertos_queue_reset(osal_queue_t handle);
osal_status_t osal_port_freertos_queue_get_count(osal_queue_t handle, uint32_t* count);
osal_status_t osal_port_freertos_queue_get_space(osal_queue_t handle, uint32_t* space);
osal_status_t osal_port_freertos_queue_is_empty(osal_queue_t handle, int* is_empty);
osal_status_t osal_port_freertos_queue_is_full(osal_queue_t handle, int* is_full);
osal_status_t osal_port_freertos_queue_get_info(osal_queue_t handle, const char** name,
                                                uint32_t* max_items, uint32_t* item_size);

/* ==================== 互斥锁适配接口 ==================== */

osal_status_t osal_port_freertos_mutex_create(const osal_mutex_config_t* config,
                                              osal_mutex_t* handle);
osal_status_t osal_port_freertos_mutex_delete(osal_mutex_t handle);
osal_status_t osal_port_freertos_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout);
osal_status_t osal_port_freertos_mutex_release(osal_mutex_t handle);
osal_status_t osal_port_freertos_mutex_try_acquire(osal_mutex_t handle);
osal_status_t osal_port_freertos_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner);
osal_status_t osal_port_freertos_mutex_get_info(osal_mutex_t handle, const char** name,
                                                uint8_t* inherit);

/* ==================== 信号量适配接口 ==================== */

osal_status_t osal_port_freertos_semaphore_create(const osal_semaphore_config_t* config,
                                                  osal_semaphore_t* handle);
osal_status_t osal_port_freertos_semaphore_delete(osal_semaphore_t handle);
osal_status_t osal_port_freertos_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout);
osal_status_t osal_port_freertos_semaphore_release(osal_semaphore_t handle);
osal_status_t osal_port_freertos_semaphore_release_from_isr(osal_semaphore_t handle,
                                                            int* higher_pri_task_woken);
osal_status_t osal_port_freertos_semaphore_try_acquire(osal_semaphore_t handle);
osal_status_t osal_port_freertos_semaphore_get_count(osal_semaphore_t handle, uint32_t* count);
osal_status_t osal_port_freertos_semaphore_set_count(osal_semaphore_t handle, uint32_t count);
osal_status_t osal_port_freertos_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                                    osal_semaphore_type_t* type,
                                                    uint32_t* max_count);

/* ==================== 事件适配接口 ==================== */

osal_status_t osal_port_freertos_event_create(const osal_event_config_t* config,
                                              osal_event_t* handle);
osal_status_t osal_port_freertos_event_delete(osal_event_t handle);
osal_status_t osal_port_freertos_event_wait(osal_event_t handle, osal_event_flags_t wait_flags,
                                            uint8_t option, osal_event_flags_t* actual_flags,
                                            osal_tick_t timeout);
osal_status_t osal_port_freertos_event_set(osal_event_t handle, osal_event_flags_t flags);
osal_status_t osal_port_freertos_event_set_from_isr(osal_event_t handle, osal_event_flags_t flags,
                                                    int* higher_pri_task_woken);
osal_status_t osal_port_freertos_event_clear(osal_event_t handle, osal_event_flags_t flags);
osal_status_t osal_port_freertos_event_get(osal_event_t handle, osal_event_flags_t* flags);
osal_status_t osal_port_freertos_event_sync(osal_event_t handle, osal_event_flags_t flags);
osal_status_t osal_port_freertos_event_get_info(osal_event_t handle, const char** name);

/* ==================== 内存管理适配接口 ==================== */

void* osal_port_freertos_memory_alloc(uint32_t size);
osal_status_t osal_port_freertos_memory_free(void* ptr);
void* osal_port_freertos_memory_realloc(void* ptr, uint32_t size);
void* osal_port_freertos_memory_calloc(uint32_t num, uint32_t size);
uint32_t osal_port_freertos_memory_get_free_size(void);
uint32_t osal_port_freertos_memory_get_minimum_free_size(void);

/* ==================== 调度器控制适配接口 ==================== */

osal_status_t osal_port_freertos_scheduler_suspend(void);
osal_status_t osal_port_freertos_scheduler_resume(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_PORT_FREERTOS_H */
