/**
 * @file osal_port.c
 * @brief RT-Thread适配层实现
 * @details 实现RT-Thread到OSAL的适配功能
 * @note 这是适配层的骨架实现，具体功能需要根据RT-Thread API完善
 */

#include "osal_port.h"

#include <stdlib.h>
#include <string.h>

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief RT-Thread错误码转换为OSAL错误码
 */
OSAL_INLINE osal_status_t rtthread_to_osal_status(rt_err_t rt_status) {
    switch (rt_status) {
        case RT_EOK:
            return OSAL_OK;
        case RT_ETIMEOUT:
            return OSAL_ERROR_TIMEOUT;
        case RT_EFULL:
        case RT_EEMPTY:
            return OSAL_ERROR_BUSY;
        case RT_ENOMEM:
            return OSAL_ERROR_NO_MEM;
        case RT_EINVAL:
            return OSAL_ERROR_INVALID_PARAM;
        default:
            return OSAL_ERROR;
    }
}

/* ==================== 初始化和反初始化 ==================== */

osal_status_t osal_port_rtthread_init(void) {
    /* RT-Thread已经初始化 */
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_deinit(void) { return OSAL_OK; }

/* ==================== 任务适配实现 ==================== */

osal_status_t osal_port_rtthread_task_create(const osal_task_config_t* config,
                                             osal_task_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL || config->func == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_thread_t task = rt_thread_create(
        config->name != NULL ? config->name : "OSAL_Task", (void (*)(void* param))config->func,
        config->param, config->stack_size, OSAL_PRIORITY_TO_RTTHREAD(config->priority),
        config->time_slice > 0 ? config->time_slice : RT_THREAD_TICK_MAX);

    if (task == RT_NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    rt_err_t ret = rt_thread_startup(task);
    if (ret != RT_EOK) {
        return rtthread_to_osal_status(ret);
    }

    *handle = RTTHREAD_TASK_HANDLE_TO_OSAL(task);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_task_delete(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_thread_t task = OSAL_TASK_HANDLE_TO_RTTHREAD(handle);
    rt_thread_delete(task);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_task_suspend(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_thread_t task = OSAL_TASK_HANDLE_TO_RTTHREAD(handle);
    rt_err_t ret = rt_thread_suspend(task);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_task_resume(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_thread_t task = OSAL_TASK_HANDLE_TO_RTTHREAD(handle);
    rt_err_t ret = rt_thread_resume(task);
    return rtthread_to_osal_status(ret);
}

osal_task_t osal_port_rtthread_task_get_current(void) {
    rt_thread_t task = rt_thread_self();
    return RTTHREAD_TASK_HANDLE_TO_OSAL(task);
}

osal_status_t osal_port_rtthread_task_yield(void) {
    rt_thread_yield();
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_task_delay(osal_tick_t ticks) {
    rt_thread_delay(OSAL_TICKS_TO_RTTHREAD(ticks));
    return OSAL_OK;
}

osal_tick_t osal_port_rtthread_task_get_tick_count(void) {
    return RTTHREAD_TICKS_TO_OSAL(rt_tick_get());
}

int osal_port_rtthread_task_is_in_isr(void) { return rt_interrupt_get_nest() != 0; }

/* 以下函数提供骨架实现，需要根据实际需求完善 */

osal_status_t osal_port_rtthread_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks) {
    (void)prev_tick;
    (void)ticks;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_task_get_priority(osal_task_t handle, osal_priority_t* priority) {
    if (priority == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
    rt_thread_t task = (handle != NULL) ? OSAL_TASK_HANDLE_TO_RTTHREAD(handle) : rt_thread_self();
    *priority = RTTHREAD_PRIORITY_TO_OSAL(task->current_priority);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_task_set_priority(osal_task_t handle, osal_priority_t priority) {
    rt_thread_t task = (handle != NULL) ? OSAL_TASK_HANDLE_TO_RTTHREAD(handle) : rt_thread_self();
    rt_err_t ret = rt_thread_control(task, RT_THREAD_CTRL_CHANGE_PRIORITY, &priority);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_task_get_state(osal_task_t handle, osal_task_state_t* state) {
    (void)handle;
    (void)state;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_task_get_info(osal_task_t handle, const char** name,
                                               uint32_t* stack_size, uint32_t* stack_free) {
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
    rt_thread_t task = OSAL_TASK_HANDLE_TO_RTTHREAD(handle);

    if (name != NULL) {
        *name = task->name;
    }

    if (stack_size != NULL) {
        *stack_size = task->stack_size;
    }

    if (stack_free != NULL) {
        *stack_free = 0; /* RT-Thread没有直接查询剩余栈的API */
    }

    return OSAL_OK;
}

/* ==================== 队列适配实现（骨架） ==================== */

osal_status_t osal_port_rtthread_queue_create(const osal_queue_config_t* config,
                                              osal_queue_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL || config->item_size == 0 || config->max_items == 0) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_queue_t queue = rt_mq_create(config->name != NULL ? config->name : "OSAL_Queue",
                                    config->item_size, config->max_items, RT_IPC_FLAG_FIFO);

    if (queue == RT_NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = RTTHREAD_QUEUE_HANDLE_TO_OSAL(queue);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_queue_delete(osal_queue_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_mq_delete(OSAL_QUEUE_HANDLE_TO_RTTHREAD(handle));
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_queue_send(osal_queue_t handle, const void* data,
                                            osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_tick_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_RTTHREAD_FOREVER
                      : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_RTTHREAD_NO_WAIT
                                                     : OSAL_TICKS_TO_RTTHREAD(timeout);

    rt_err_t ret = rt_mq_send_wait(OSAL_QUEUE_HANDLE_TO_RTTHREAD(handle), data, ticks);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_queue_send_front(osal_queue_t handle, const void* data,
                                                  osal_tick_t timeout) {
    /* RT-Thread消息队列不支持前端发送，返回不支持 */
    (void)handle;
    (void)data;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_queue_receive(osal_queue_t handle, void* data,
                                               osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_tick_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_RTTHREAD_FOREVER
                      : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_RTTHREAD_NO_WAIT
                                                     : OSAL_TICKS_TO_RTTHREAD(timeout);

    rt_err_t ret = rt_mq_recv(OSAL_QUEUE_HANDLE_TO_RTTHREAD(handle), data, 0, ticks);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_queue_send_from_isr(osal_queue_t handle, const void* data,
                                                     int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_mq_send(OSAL_QUEUE_HANDLE_TO_RTTHREAD(handle), data, 0);
    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (ret == RT_EOK) ? 1 : 0;
    }
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_queue_receive_from_isr(osal_queue_t handle, void* data,
                                                        int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || data == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret =
        rt_mq_recv(OSAL_QUEUE_HANDLE_TO_RTTHREAD(handle), data, 0, OSAL_WAIT_RTTHREAD_NO_WAIT);
    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (ret == RT_EOK) ? 1 : 0;
    }
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_queue_reset(osal_queue_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_queue_get_count(osal_queue_t handle, uint32_t* count) {
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_queue_get_space(osal_queue_t handle, uint32_t* space) {
    (void)handle;
    (void)space;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_queue_is_empty(osal_queue_t handle, int* is_empty) {
    (void)handle;
    (void)is_empty;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_queue_is_full(osal_queue_t handle, int* is_full) {
    (void)handle;
    (void)is_full;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_queue_get_info(osal_queue_t handle, const char** name,
                                                uint32_t* max_items, uint32_t* item_size) {
    (void)handle;
    (void)name;
    (void)max_items;
    (void)item_size;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 互斥锁适配实现（骨架） ==================== */

osal_status_t osal_port_rtthread_mutex_create(const osal_mutex_config_t* config,
                                              osal_mutex_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_mutex_t mutex = rt_mutex_create(
        config != NULL && config->name != NULL ? config->name : "OSAL_Mutex", RT_IPC_FLAG_FIFO);

    if (mutex == RT_NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = RTTHREAD_MUTEX_HANDLE_TO_OSAL(mutex);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_mutex_delete(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_mutex_delete(OSAL_MUTEX_HANDLE_TO_RTTHREAD(handle));
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_tick_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_RTTHREAD_FOREVER
                      : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_RTTHREAD_NO_WAIT
                                                     : OSAL_TICKS_TO_RTTHREAD(timeout);

    rt_err_t ret = rt_mutex_take(OSAL_MUTEX_HANDLE_TO_RTTHREAD(handle), ticks);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_mutex_release(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_mutex_release(OSAL_MUTEX_HANDLE_TO_RTTHREAD(handle));
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_mutex_try_acquire(osal_mutex_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_mutex_take(OSAL_MUTEX_HANDLE_TO_RTTHREAD(handle), OSAL_WAIT_RTTHREAD_NO_WAIT);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || owner == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_mutex_t mutex = OSAL_MUTEX_HANDLE_TO_RTTHREAD(handle);
    *owner = RTTHREAD_TASK_HANDLE_TO_OSAL(mutex->owner);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_mutex_get_info(osal_mutex_t handle, const char** name,
                                                uint8_t* inherit) {
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }

    rt_mutex_t mutex = OSAL_MUTEX_HANDLE_TO_RTTHREAD(handle);

    if (name != NULL) {
        *name = mutex->parent.parent.name;
    }

    if (inherit != NULL) {
        *inherit = 1; /* RT-Thread互斥锁支持优先级继承 */
    }

    return OSAL_OK;
}

/* ==================== 信号量适配实现（骨架） ==================== */

osal_status_t osal_port_rtthread_semaphore_create(const osal_semaphore_config_t* config,
                                                  osal_semaphore_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_sem_t sem = rt_sem_create(config->name != NULL ? config->name : "OSAL_Sem",
                                 config->init_count, RT_IPC_FLAG_FIFO);

    if (sem == RT_NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = RTTHREAD_SEMAPHORE_HANDLE_TO_OSAL(sem);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_semaphore_delete(osal_semaphore_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_sem_delete(OSAL_SEMAPHORE_HANDLE_TO_RTTHREAD(handle));
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_tick_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_RTTHREAD_FOREVER
                      : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_RTTHREAD_NO_WAIT
                                                     : OSAL_TICKS_TO_RTTHREAD(timeout);

    rt_err_t ret = rt_sem_take(OSAL_SEMAPHORE_HANDLE_TO_RTTHREAD(handle), ticks);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_semaphore_release(osal_semaphore_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_sem_release(OSAL_SEMAPHORE_HANDLE_TO_RTTHREAD(handle));
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_semaphore_release_from_isr(osal_semaphore_t handle,
                                                            int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_sem_release(OSAL_SEMAPHORE_HANDLE_TO_RTTHREAD(handle));
    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (ret == RT_EOK) ? 1 : 0;
    }
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_semaphore_try_acquire(osal_semaphore_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret =
        rt_sem_take(OSAL_SEMAPHORE_HANDLE_TO_RTTHREAD(handle), OSAL_WAIT_RTTHREAD_NO_WAIT);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_semaphore_get_count(osal_semaphore_t handle, uint32_t* count) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || count == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_sem_t sem = OSAL_SEMAPHORE_HANDLE_TO_RTTHREAD(handle);
    *count = sem->count;
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_semaphore_set_count(osal_semaphore_t handle, uint32_t count) {
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_rtthread_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                                    osal_semaphore_type_t* type,
                                                    uint32_t* max_count) {
    (void)handle;
    (void)name;
    (void)type;
    (void)max_count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 事件适配实现（骨架） ==================== */

osal_status_t osal_port_rtthread_event_create(const osal_event_config_t* config,
                                              osal_event_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_event_t event = rt_event_create(
        config != NULL && config->name != NULL ? config->name : "OSAL_Event", RT_IPC_FLAG_FIFO);

    if (event == RT_NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    *handle = RTTHREAD_EVENT_HANDLE_TO_OSAL(event);
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_event_delete(osal_event_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_event_delete(OSAL_EVENT_HANDLE_TO_RTTHREAD(handle));
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_event_wait(osal_event_t handle, osal_event_flags_t wait_flags,
                                            uint8_t option, osal_event_flags_t* actual_flags,
                                            osal_tick_t timeout) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_tick_t ticks = (timeout == OSAL_WAIT_FOREVER) ? OSAL_WAIT_RTTHREAD_FOREVER
                      : (timeout == OSAL_NO_WAIT)    ? OSAL_WAIT_RTTHREAD_NO_WAIT
                                                     : OSAL_TICKS_TO_RTTHREAD(timeout);

    rt_uint8_t opt = (option & OSAL_EVENT_WAIT_ALL) ? RT_EVENT_FLAG_AND : RT_EVENT_FLAG_OR;
    if (option & OSAL_EVENT_WAIT_CLEAR) {
        opt |= RT_EVENT_FLAG_CLEAR;
    }

    rt_err_t ret = rt_event_recv(OSAL_EVENT_HANDLE_TO_RTTHREAD(handle), wait_flags, opt, ticks,
                                 (rt_uint32_t*)actual_flags);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_event_set(osal_event_t handle, osal_event_flags_t flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_event_send(OSAL_EVENT_HANDLE_TO_RTTHREAD(handle), flags);
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_event_set_from_isr(osal_event_t handle, osal_event_flags_t flags,
                                                    int* higher_pri_task_woken) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_err_t ret = rt_event_send(OSAL_EVENT_HANDLE_TO_RTTHREAD(handle), flags);
    if (higher_pri_task_woken != NULL) {
        *higher_pri_task_woken = (ret == RT_EOK) ? 1 : 0;
    }
    return rtthread_to_osal_status(ret);
}

osal_status_t osal_port_rtthread_event_clear(osal_event_t handle, osal_event_flags_t flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_event_t event = OSAL_EVENT_HANDLE_TO_RTTHREAD(handle);
    event->set &= ~flags;
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_event_get(osal_event_t handle, osal_event_flags_t* flags) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL || flags == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_event_t event = OSAL_EVENT_HANDLE_TO_RTTHREAD(handle);
    *flags = event->set;
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_event_sync(osal_event_t handle, osal_event_flags_t flags) {
    return osal_port_rtthread_event_set(handle, flags);
}

osal_status_t osal_port_rtthread_event_get_info(osal_event_t handle, const char** name) {
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }

    rt_event_t event = OSAL_EVENT_HANDLE_TO_RTTHREAD(handle);
    if (name != NULL) {
        *name = event->parent.parent.name;
    }

    return OSAL_OK;
}

/* ==================== 内存管理适配实现 ==================== */

void* osal_port_rtthread_memory_alloc(uint32_t size) { return rt_malloc(size); }

osal_status_t osal_port_rtthread_memory_free(void* ptr) {
#if OSAL_CFG_PARAM_CHECK
    if (ptr == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    rt_free(ptr);
    return OSAL_OK;
}

void* osal_port_rtthread_memory_realloc(void* ptr, uint32_t size) { return rt_realloc(ptr, size); }

void* osal_port_rtthread_memory_calloc(uint32_t num, uint32_t size) { return rt_calloc(num, size); }

uint32_t osal_port_rtthread_memory_get_free_size(void) { return rt_memory_info(RT_NULL); }

uint32_t osal_port_rtthread_memory_get_minimum_free_size(void) {
    return 0; /* RT-Thread没有直接API */
}

/* ==================== 调度器控制适配实现 ==================== */

osal_status_t osal_port_rtthread_scheduler_suspend(void) {
    rt_enter_critical();
    return OSAL_OK;
}

osal_status_t osal_port_rtthread_scheduler_resume(void) {
    rt_exit_critical();
    return OSAL_OK;
}
