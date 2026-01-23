/**
 * @file osal_port.c
 * @brief uC/OS-II适配层实现
 * @details 实现uC/OS-II到OSAL的适配功能
 * @note 这是适配层的骨架实现，具体功能需要根据uC/OS-II API完善
 */

#include "osal_port.h"

#include <stdlib.h>
#include <string.h>

/* ==================== 内部数据结构 ==================== */

/* uC/OS-II任务包装结构 */
typedef struct {
    osal_task_func_t func;
    void* param;
} osal_ucos_task_wrapper_t;

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief uC/OS-II错误码转换为OSAL错误码
 */
OSAL_INLINE osal_status_t ucos_to_osal_status(INT8U ucos_status) {
    switch (ucos_status) {
        case OS_ERR_NONE:
            return OSAL_OK;
        case OS_ERR_TIMEOUT:
            return OSAL_ERROR_TIMEOUT;
        case OS_ERR_PEVENT_NULL:
        case OS_ERR_INVALID_PID:
            return OSAL_ERROR_INVALID_PARAM;
        case OS_ERR_NO_MORE_TCB:
        case OS_ERR_MEM_FULL:
            return OSAL_ERROR_NO_MEM;
        default:
            return OSAL_ERROR;
    }
}

/**
 * @brief uC/OS-II任务包装函数
 */
static void osal_ucos_task_wrapper(void* pdata) {
    osal_ucos_task_wrapper_t* wrapper = (osal_ucos_task_wrapper_t*)pdata;

    if (wrapper != NULL && wrapper->func != NULL) {
        wrapper->func(wrapper->param);
    }

    /* 任务结束，删除自己 */
    OSTaskDel(OS_PRIO_SELF);
}

/* ==================== 初始化和反初始化 ==================== */

osal_status_t osal_port_ucos_init(void) {
    /* uC/OS-II由OSInit()初始化，在这里不需要做额外操作 */
    return OSAL_OK;
}

osal_status_t osal_port_ucos_deinit(void) { return OSAL_OK; }

/* ==================== 任务适配实现 ==================== */

osal_status_t osal_port_ucos_task_create(const osal_task_config_t* config, osal_task_t* handle) {
#if OSAL_CFG_PARAM_CHECK
    if (config == NULL || handle == NULL || config->func == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    /* 分配任务包装结构 */
    osal_ucos_task_wrapper_t* wrapper =
        (osal_ucos_task_wrapper_t*)malloc(sizeof(osal_ucos_task_wrapper_t));
    if (wrapper == NULL) {
        return OSAL_ERROR_NO_MEM;
    }

    wrapper->func = config->func;
    wrapper->param = config->param;

    /* 分配栈空间 */
    OS_STK* stack = (OS_STK*)malloc(config->stack_size);
    if (stack == NULL) {
        free(wrapper);
        return OSAL_ERROR_NO_MEM;
    }

    /* 创建任务 */
    INT8U prio = OSAL_PRIORITY_TO_UCOS(config->priority);
    INT8U err = OSTaskCreateExt(osal_ucos_task_wrapper, wrapper,
                                &stack[config->stack_size / sizeof(OS_STK) - 1], prio, prio, stack,
                                config->stack_size / sizeof(OS_STK), NULL,
                                OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

    if (err != OS_ERR_NONE) {
        free(stack);
        free(wrapper);
        return ucos_to_osal_status(err);
    }

    *handle = (osal_task_t)(intptr_t)prio;
    return OSAL_OK;
}

osal_status_t osal_port_ucos_task_delete(osal_task_t handle) {
#if OSAL_CFG_PARAM_CHECK
    if (handle == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    INT8U prio = (INT8U)(intptr_t)handle;
    INT8U err = OSTaskDel(prio);

    /* 注意：这里需要释放任务包装结构和栈空间，但由于uC/OS-II限制，简化处理 */
    return ucos_to_osal_status(err);
}

osal_task_t osal_port_ucos_task_get_current(void) {
    INT8U prio = OSPrioCur;
    return (osal_task_t)(intptr_t)prio;
}

osal_status_t osal_port_ucos_task_yield(void) {
    OSSchedRoundRobin();
    return OSAL_OK;
}

osal_status_t osal_port_ucos_task_delay(osal_tick_t ticks) {
    OSTimeDlyHMSM(0, 0, 0, ticks);
    return OSAL_OK;
}

osal_tick_t osal_port_ucos_task_get_tick_count(void) { return OSTimeGet(); }

int osal_port_ucos_task_is_in_isr(void) { return OSIntNesting > 0; }

/* 以下函数提供骨架实现，需要根据实际需求完善 */

osal_status_t osal_port_ucos_task_suspend(osal_task_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_task_resume(osal_task_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_task_delay_until(osal_tick_t* prev_tick, osal_tick_t ticks) {
    (void)prev_tick;
    (void)ticks;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_task_get_priority(osal_task_t handle, osal_priority_t* priority) {
    if (priority == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
    INT8U prio = (INT8U)(intptr_t)handle;
    *priority = UCOS_PRIORITY_TO_OSAL(prio);
    return OSAL_OK;
}

osal_status_t osal_port_ucos_task_set_priority(osal_task_t handle, osal_priority_t priority) {
    (void)handle;
    (void)priority;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_task_get_state(osal_task_t handle, osal_task_state_t* state) {
    (void)handle;
    (void)state;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_task_get_info(osal_task_t handle, const char** name,
                                           uint32_t* stack_size, uint32_t* stack_free) {
    (void)handle;
    (void)name;
    (void)stack_size;
    (void)stack_free;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 队列适配实现（骨架） ==================== */

osal_status_t osal_port_ucos_queue_create(const osal_queue_config_t* config, osal_queue_t* handle) {
    (void)config;
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_delete(osal_queue_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_send(osal_queue_t handle, const void* data,
                                        osal_tick_t timeout) {
    (void)handle;
    (void)data;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_send_front(osal_queue_t handle, const void* data,
                                              osal_tick_t timeout) {
    (void)handle;
    (void)data;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_receive(osal_queue_t handle, void* data, osal_tick_t timeout) {
    (void)handle;
    (void)data;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_send_from_isr(osal_queue_t handle, const void* data,
                                                 int* higher_pri_task_woken) {
    (void)handle;
    (void)data;
    (void)higher_pri_task_woken;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_receive_from_isr(osal_queue_t handle, void* data,
                                                    int* higher_pri_task_woken) {
    (void)handle;
    (void)data;
    (void)higher_pri_task_woken;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_reset(osal_queue_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_get_count(osal_queue_t handle, uint32_t* count) {
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_get_space(osal_queue_t handle, uint32_t* space) {
    (void)handle;
    (void)space;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_is_empty(osal_queue_t handle, int* is_empty) {
    (void)handle;
    (void)is_empty;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_is_full(osal_queue_t handle, int* is_full) {
    (void)handle;
    (void)is_full;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_queue_get_info(osal_queue_t handle, const char** name,
                                            uint32_t* max_items, uint32_t* item_size) {
    (void)handle;
    (void)name;
    (void)max_items;
    (void)item_size;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 互斥锁适配实现（骨架） ==================== */

osal_status_t osal_port_ucos_mutex_create(const osal_mutex_config_t* config, osal_mutex_t* handle) {
    (void)config;
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_mutex_delete(osal_mutex_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_mutex_acquire(osal_mutex_t handle, osal_tick_t timeout) {
    (void)handle;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_mutex_release(osal_mutex_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_mutex_try_acquire(osal_mutex_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_mutex_get_owner(osal_mutex_t handle, osal_task_t* owner) {
    (void)handle;
    (void)owner;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_mutex_get_info(osal_mutex_t handle, const char** name,
                                            uint8_t* inherit) {
    (void)handle;
    (void)name;
    (void)inherit;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 信号量适配实现（骨架） ==================== */

osal_status_t osal_port_ucos_semaphore_create(const osal_semaphore_config_t* config,
                                              osal_semaphore_t* handle) {
    (void)config;
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_delete(osal_semaphore_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_acquire(osal_semaphore_t handle, osal_tick_t timeout) {
    (void)handle;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_release(osal_semaphore_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_release_from_isr(osal_semaphore_t handle,
                                                        int* higher_pri_task_woken) {
    (void)handle;
    (void)higher_pri_task_woken;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_try_acquire(osal_semaphore_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_get_count(osal_semaphore_t handle, uint32_t* count) {
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_set_count(osal_semaphore_t handle, uint32_t count) {
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_semaphore_get_info(osal_semaphore_t handle, const char** name,
                                                osal_semaphore_type_t* type, uint32_t* max_count) {
    (void)handle;
    (void)name;
    (void)type;
    (void)max_count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 事件适配实现（骨架） ==================== */

osal_status_t osal_port_ucos_event_create(const osal_event_config_t* config, osal_event_t* handle) {
    (void)config;
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_delete(osal_event_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_wait(osal_event_t handle, osal_event_flags_t wait_flags,
                                        uint8_t option, osal_event_flags_t* actual_flags,
                                        osal_tick_t timeout) {
    (void)handle;
    (void)wait_flags;
    (void)option;
    (void)actual_flags;
    (void)timeout;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_set(osal_event_t handle, osal_event_flags_t flags) {
    (void)handle;
    (void)flags;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_set_from_isr(osal_event_t handle, osal_event_flags_t flags,
                                                int* higher_pri_task_woken) {
    (void)handle;
    (void)flags;
    (void)higher_pri_task_woken;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_clear(osal_event_t handle, osal_event_flags_t flags) {
    (void)handle;
    (void)flags;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_get(osal_event_t handle, osal_event_flags_t* flags) {
    (void)handle;
    (void)flags;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_sync(osal_event_t handle, osal_event_flags_t flags) {
    (void)handle;
    (void)flags;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_port_ucos_event_get_info(osal_event_t handle, const char** name) {
    (void)handle;
    (void)name;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 内存管理适配实现 ==================== */

void* osal_port_ucos_memory_alloc(uint32_t size) {
    /* uC/OS-II内存管理使用OSMem */
    (void)size;
    return malloc(size); /* 使用标准库作为后备 */
}

osal_status_t osal_port_ucos_memory_free(void* ptr) {
    if (ptr == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
    free(ptr);
    return OSAL_OK;
}

void* osal_port_ucos_memory_realloc(void* ptr, uint32_t size) {
    (void)ptr;
    (void)size;
    return NULL;
}

void* osal_port_ucos_memory_calloc(uint32_t num, uint32_t size) {
    void* ptr = malloc(num * size);
    if (ptr != NULL) {
        memset(ptr, 0, num * size);
    }
    return ptr;
}

uint32_t osal_port_ucos_memory_get_free_size(void) {
    return 0; /* uC/OS-II需要配置内存管理后才能查询 */
}

uint32_t osal_port_ucos_memory_get_minimum_free_size(void) { return 0; }

/* ==================== 调度器控制适配实现 ==================== */

osal_status_t osal_port_ucos_scheduler_suspend(void) {
    OSSchedLock();
    return OSAL_OK;
}

osal_status_t osal_port_ucos_scheduler_resume(void) {
    OSSchedUnlock();
    return OSAL_OK;
}
