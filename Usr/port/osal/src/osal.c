/**
 * @file osal.c
 * @brief OSAL核心实现
 * @details 实现OSAL的初始化、控制、工厂模式和单例模式
 */

#include "osal.h"

#include <stdlib.h>
#include <string.h>

#include "osal_ops.h"

/* ==================== 引入各适配层 ==================== */

#if OSAL_OS_TYPE == OSAL_OS_FREERTOS || OSAL_CFG_USE_FUNCTION_PTR
#include "../port/freertos/osal_port.h"
#elif OSAL_OS_TYPE == OSAL_OS_UCOS_II || OSAL_CFG_USE_FUNCTION_PTR
#include "../port/ucos_ii/osal_port.h"
#elif OSAL_OS_TYPE == OSAL_OS_RTTHREAD || OSAL_CFG_USE_FUNCTION_PTR
#include "../port/rtthread/osal_port.h"
#endif

/* ==================== OSAL单例结构 ==================== */

/* 单例实例 */
static osal_instance_t g_osal_instance = {.state = OSAL_STATE_UNINITIALIZED,
                                          .os_type = OSAL_OS_NONE,
                                          .port = NULL,
                                          .task_count = 0,
                                          .queue_count = 0,
                                          .mutex_count = 0,
                                          .semaphore_count = 0,
                                          .event_count = 0,
                                          .tick_rate_hz = OSAL_CFG_TICK_RATE_HZ};

/* ==================== 操作接口定义 ==================== */

/* FreeRTOS操作接口 */
#if OSAL_OS_TYPE == OSAL_OS_FREERTOS || OSAL_CFG_USE_FUNCTION_PTR

static const osal_task_ops_t osal_freertos_task_ops = {
    .create = osal_port_freertos_task_create,
    .delete = osal_port_freertos_task_delete,
    .suspend = osal_port_freertos_task_suspend,
    .resume = osal_port_freertos_task_resume,
    .get_current = osal_port_freertos_task_get_current,
    .yield = osal_port_freertos_task_yield,
    .delay = osal_port_freertos_task_delay,
    .delay_until = osal_port_freertos_task_delay_until,
    .get_priority = osal_port_freertos_task_get_priority,
    .set_priority = osal_port_freertos_task_set_priority,
    .get_state = osal_port_freertos_task_get_state,
    .get_info = osal_port_freertos_task_get_info,
    .get_tick_count = osal_port_freertos_task_get_tick_count,
    .is_in_isr = osal_port_freertos_task_is_in_isr,
};

static const osal_queue_ops_t osal_freertos_queue_ops = {
    .create = osal_port_freertos_queue_create,
    .delete = osal_port_freertos_queue_delete,
    .send = osal_port_freertos_queue_send,
    .send_front = osal_port_freertos_queue_send_front,
    .receive = osal_port_freertos_queue_receive,
    .reset = osal_port_freertos_queue_reset,
    .get_count = osal_port_freertos_queue_get_count,
    .get_space = osal_port_freertos_queue_get_space,
    .is_empty = osal_port_freertos_queue_is_empty,
    .is_full = osal_port_freertos_queue_is_full,
    .get_info = osal_port_freertos_queue_get_info,
};

static const osal_mutex_ops_t osal_freertos_mutex_ops = {
    .create = osal_port_freertos_mutex_create,
    .delete = osal_port_freertos_mutex_delete,
    .acquire = osal_port_freertos_mutex_acquire,
    .release = osal_port_freertos_mutex_release,
    .get_owner = osal_port_freertos_mutex_get_owner,
    .get_info = osal_port_freertos_mutex_get_info,
};

static const osal_semaphore_ops_t osal_freertos_semaphore_ops = {
    .create = osal_port_freertos_semaphore_create,
    .delete = osal_port_freertos_semaphore_delete,
    .acquire = osal_port_freertos_semaphore_acquire,
    .release = osal_port_freertos_semaphore_release,
    .get_count = osal_port_freertos_semaphore_get_count,
    .set_count = osal_port_freertos_semaphore_set_count,
    .get_info = osal_port_freertos_semaphore_get_info,
};

static const osal_event_ops_t osal_freertos_event_ops = {
    .create = osal_port_freertos_event_create,
    .delete = osal_port_freertos_event_delete,
    .wait = osal_port_freertos_event_wait,
    .set = osal_port_freertos_event_set,
    .clear = osal_port_freertos_event_clear,
    .get = osal_port_freertos_event_get,
    .sync = osal_port_freertos_event_sync,
    .get_info = osal_port_freertos_event_get_info,
};

static const osal_memory_ops_t osal_freertos_memory_ops = {
    .alloc = osal_port_freertos_memory_alloc,
    .free = osal_port_freertos_memory_free,
    .realloc = osal_port_freertos_memory_realloc,
    .calloc = osal_port_freertos_memory_calloc,
    .get_free_size = osal_port_freertos_memory_get_free_size,
    .get_minimum_free_size = osal_port_freertos_memory_get_minimum_free_size,
};

static const osal_scheduler_ops_t osal_freertos_scheduler_ops = {
    .suspend = osal_port_freertos_scheduler_suspend,
    .resume = osal_port_freertos_scheduler_resume,
};

static const osal_ops_t osal_freertos_ops = {
    .task = &osal_freertos_task_ops,
    .queue = &osal_freertos_queue_ops,
    .mutex = &osal_freertos_mutex_ops,
    .semaphore = &osal_freertos_semaphore_ops,
    .event = &osal_freertos_event_ops,
    .memory = &osal_freertos_memory_ops,
    .scheduler = &osal_freertos_scheduler_ops,
};

/* OSAL_OS_FREERTOS */

/* uC/OS-II操作接口 */
#elif OSAL_OS_TYPE == OSAL_OS_UCOS_II || OSAL_CFG_USE_FUNCTION_PTR

static const osal_task_ops_t osal_ucos_task_ops = {
    .create = osal_port_ucos_task_create,
    .delete = osal_port_ucos_task_delete,
    .suspend = osal_port_ucos_task_suspend,
    .resume = osal_port_ucos_task_resume,
    .get_current = osal_port_ucos_task_get_current,
    .yield = osal_port_ucos_task_yield,
    .delay = osal_port_ucos_task_delay,
    .delay_until = osal_port_ucos_task_delay_until,
    .get_priority = osal_port_ucos_task_get_priority,
    .set_priority = osal_port_ucos_task_set_priority,
    .get_state = osal_port_ucos_task_get_state,
    .get_info = osal_port_ucos_task_get_info,
    .get_tick_count = osal_port_ucos_task_get_tick_count,
    .is_in_isr = osal_port_ucos_task_is_in_isr,
};

static const osal_queue_ops_t osal_ucos_queue_ops = {
    .create = osal_port_ucos_queue_create,
    .delete = osal_port_ucos_queue_delete,
    .send = osal_port_ucos_queue_send,
    .send_front = osal_port_ucos_queue_send_front,
    .receive = osal_port_ucos_queue_receive,
    .send_from_isr = osal_port_ucos_queue_send_from_isr,
    .receive_from_isr = osal_port_ucos_queue_receive_from_isr,
    .reset = osal_port_ucos_queue_reset,
    .get_count = osal_port_ucos_queue_get_count,
    .get_space = osal_port_ucos_queue_get_space,
    .is_empty = osal_port_ucos_queue_is_empty,
    .is_full = osal_port_ucos_queue_is_full,
    .get_info = osal_port_ucos_queue_get_info,
};

static const osal_mutex_ops_t osal_ucos_mutex_ops = {
    .create = osal_port_ucos_mutex_create,
    .delete = osal_port_ucos_mutex_delete,
    .acquire = osal_port_ucos_mutex_acquire,
    .release = osal_port_ucos_mutex_release,
    .try_acquire = osal_port_ucos_mutex_try_acquire,
    .get_owner = osal_port_ucos_mutex_get_owner,
    .get_info = osal_port_ucos_mutex_get_info,
};

static const osal_semaphore_ops_t osal_ucos_semaphore_ops = {
    .create = osal_port_ucos_semaphore_create,
    .delete = osal_port_ucos_semaphore_delete,
    .acquire = osal_port_ucos_semaphore_acquire,
    .release = osal_port_ucos_semaphore_release,
    .release_from_isr = osal_port_ucos_semaphore_release_from_isr,
    .try_acquire = osal_port_ucos_semaphore_try_acquire,
    .get_count = osal_port_ucos_semaphore_get_count,
    .set_count = osal_port_ucos_semaphore_set_count,
    .get_info = osal_port_ucos_semaphore_get_info,
};

static const osal_event_ops_t osal_ucos_event_ops = {
    .create = osal_port_ucos_event_create,
    .delete = osal_port_ucos_event_delete,
    .wait = osal_port_ucos_event_wait,
    .set = osal_port_ucos_event_set,
    .set_from_isr = osal_port_ucos_event_set_from_isr,
    .clear = osal_port_ucos_event_clear,
    .get = osal_port_ucos_event_get,
    .sync = osal_port_ucos_event_sync,
    .get_info = osal_port_ucos_event_get_info,
};

static const osal_memory_ops_t osal_ucos_memory_ops = {
    .alloc = osal_port_ucos_memory_alloc,
    .free = osal_port_ucos_memory_free,
    .realloc = osal_port_ucos_memory_realloc,
    .calloc = osal_port_ucos_memory_calloc,
    .get_free_size = osal_port_ucos_memory_get_free_size,
    .get_minimum_free_size = osal_port_ucos_memory_get_minimum_free_size,
};

static const osal_scheduler_ops_t osal_ucos_scheduler_ops = {
    .suspend = osal_port_ucos_scheduler_suspend,
    .resume = osal_port_ucos_scheduler_resume,
};

static const osal_ops_t osal_ucos_ops = {
    .task = &osal_ucos_task_ops,
    .queue = &osal_ucos_queue_ops,
    .mutex = &osal_ucos_mutex_ops,
    .semaphore = &osal_ucos_semaphore_ops,
    .event = &osal_ucos_event_ops,
    .memory = &osal_ucos_memory_ops,
    .scheduler = &osal_ucos_scheduler_ops,
};

/* OSAL_OS_UCOS_II */

/* RT-Thread操作接口 */
#elif OSAL_OS_TYPE == OSAL_OS_RTTHREAD || OSAL_CFG_USE_FUNCTION_PTR

static const osal_task_ops_t osal_rtthread_task_ops = {
    .create = osal_port_rtthread_task_create,
    .delete = osal_port_rtthread_task_delete,
    .suspend = osal_port_rtthread_task_suspend,
    .resume = osal_port_rtthread_task_resume,
    .get_current = osal_port_rtthread_task_get_current,
    .yield = osal_port_rtthread_task_yield,
    .delay = osal_port_rtthread_task_delay,
    .delay_until = osal_port_rtthread_task_delay_until,
    .get_priority = osal_port_rtthread_task_get_priority,
    .set_priority = osal_port_rtthread_task_set_priority,
    .get_state = osal_port_rtthread_task_get_state,
    .get_info = osal_port_rtthread_task_get_info,
    .get_tick_count = osal_port_rtthread_task_get_tick_count,
    .is_in_isr = osal_port_rtthread_task_is_in_isr,
};

static const osal_queue_ops_t osal_rtthread_queue_ops = {
    .create = osal_port_rtthread_queue_create,
    .delete = osal_port_rtthread_queue_delete,
    .send = osal_port_rtthread_queue_send,
    .send_front = osal_port_rtthread_queue_send_front,
    .receive = osal_port_rtthread_queue_receive,
    .send_from_isr = osal_port_rtthread_queue_send_from_isr,
    .receive_from_isr = osal_port_rtthread_queue_receive_from_isr,
    .reset = osal_port_rtthread_queue_reset,
    .get_count = osal_port_rtthread_queue_get_count,
    .get_space = osal_port_rtthread_queue_get_space,
    .is_empty = osal_port_rtthread_queue_is_empty,
    .is_full = osal_port_rtthread_queue_is_full,
    .get_info = osal_port_rtthread_queue_get_info,
};

static const osal_mutex_ops_t osal_rtthread_mutex_ops = {
    .create = osal_port_rtthread_mutex_create,
    .delete = osal_port_rtthread_mutex_delete,
    .acquire = osal_port_rtthread_mutex_acquire,
    .release = osal_port_rtthread_mutex_release,
    .try_acquire = osal_port_rtthread_mutex_try_acquire,
    .get_owner = osal_port_rtthread_mutex_get_owner,
    .get_info = osal_port_rtthread_mutex_get_info,
};

static const osal_semaphore_ops_t osal_rtthread_semaphore_ops = {
    .create = osal_port_rtthread_semaphore_create,
    .delete = osal_port_rtthread_semaphore_delete,
    .acquire = osal_port_rtthread_semaphore_acquire,
    .release = osal_port_rtthread_semaphore_release,
    .release_from_isr = osal_port_rtthread_semaphore_release_from_isr,
    .try_acquire = osal_port_rtthread_semaphore_try_acquire,
    .get_count = osal_port_rtthread_semaphore_get_count,
    .set_count = osal_port_rtthread_semaphore_set_count,
    .get_info = osal_port_rtthread_semaphore_get_info,
};

static const osal_event_ops_t osal_rtthread_event_ops = {
    .create = osal_port_rtthread_event_create,
    .delete = osal_port_rtthread_event_delete,
    .wait = osal_port_rtthread_event_wait,
    .set = osal_port_rtthread_event_set,
    .set_from_isr = osal_port_rtthread_event_set_from_isr,
    .clear = osal_port_rtthread_event_clear,
    .get = osal_port_rtthread_event_get,
    .sync = osal_port_rtthread_event_sync,
    .get_info = osal_port_rtthread_event_get_info,
};

static const osal_memory_ops_t osal_rtthread_memory_ops = {
    .alloc = osal_port_rtthread_memory_alloc,
    .free = osal_port_rtthread_memory_free,
    .realloc = osal_port_rtthread_memory_realloc,
    .calloc = osal_port_rtthread_memory_calloc,
    .get_free_size = osal_port_rtthread_memory_get_free_size,
    .get_minimum_free_size = osal_port_rtthread_memory_get_minimum_free_size,
};

static const osal_scheduler_ops_t osal_rtthread_scheduler_ops = {
    .suspend = osal_port_rtthread_scheduler_suspend,
    .resume = osal_port_rtthread_scheduler_resume,
};

static const osal_ops_t osal_rtthread_ops = {
    .task = &osal_rtthread_task_ops,
    .queue = &osal_rtthread_queue_ops,
    .mutex = &osal_rtthread_mutex_ops,
    .semaphore = &osal_rtthread_semaphore_ops,
    .event = &osal_rtthread_event_ops,
    .memory = &osal_rtthread_memory_ops,
    .scheduler = &osal_rtthread_scheduler_ops,
};

#endif /* OSAL_OS_RTTHREAD */

/* ==================== 适配层描述表（工厂模式） ==================== */

/**
 * @brief 适配层描述表
 * @details 工厂模式：根据OS类型选择对应的适配层
 */
static const osal_port_desc_t g_osal_port_desc_table[] = {
#if OSAL_OS_TYPE == OSAL_OS_FREERTOS || OSAL_CFG_USE_FUNCTION_PTR
    {
        .os_type = OSAL_OS_FREERTOS,
        .name = "FreeRTOS",
        .ops = &osal_freertos_ops,
        .init = osal_port_freertos_init,
        .deinit = osal_port_freertos_deinit,
    },
#elif OSAL_OS_TYPE == OSAL_OS_UCOS_II || OSAL_CFG_USE_FUNCTION_PTR
    {
        .os_type = OSAL_OS_UCOS_II,
        .name = "uC/OS-II",
        .ops = &osal_ucos_ops,
        .init = osal_port_ucos_init,
        .deinit = osal_port_ucos_deinit,
    },
#elif OSAL_OS_TYPE == OSAL_OS_RTTHREAD || OSAL_CFG_USE_FUNCTION_PTR
    {
        .os_type = OSAL_OS_RTTHREAD,
        .name = "RT-Thread",
        .ops = &osal_rtthread_ops,
        .init = osal_port_rtthread_init,
        .deinit = osal_port_rtthread_deinit,
    },
#endif
};

#define OSAL_PORT_DESC_TABLE_SIZE \
    (sizeof(g_osal_port_desc_table) / sizeof(g_osal_port_desc_table[0]))

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 获取OSAL实例（单例模式，内部使用）
 */
osal_instance_t* osal_get_instance_internal(void) { return &g_osal_instance; }

/**
 * @brief 获取OSAL实例（单例模式）
 */
OSAL_INLINE osal_instance_t* osal_get_instance(void) { return &g_osal_instance; }

/**
 * @brief 获取指定OS类型的适配层描述（工厂模式）
 */
const osal_port_desc_t* osal_get_port_desc(osal_os_type_t os_type) {
    for (size_t i = 0; i < OSAL_PORT_DESC_TABLE_SIZE; i++) {
        if (g_osal_port_desc_table[i].os_type == os_type) {
            return &g_osal_port_desc_table[i];
        }
    }
    return NULL;
}

/**
 * @brief 获取OSAL操作接口（内部使用）
 */
const osal_ops_t* osal_get_ops(void) {
    osal_instance_t* instance = osal_get_instance();
    if (instance->port == NULL || instance->port->ops == NULL) {
        return NULL;
    }
    return instance->port->ops;
}

/**
 * @brief 获取当前OSAL适配层描述（内部使用）
 */
const osal_port_desc_t* osal_get_current_port(void) {
    osal_instance_t* instance = osal_get_instance();
    return instance->port;
}

/* ==================== OSAL初始化和控制接口 ==================== */



osal_status_t osal_init(osal_os_type_t os_type) {
    osal_instance_t* instance = osal_get_instance();

#if OSAL_CFG_PARAM_CHECK
    if (os_type < OSAL_OS_NONE || os_type > OSAL_OS_RTTHREAD) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    /* 检查是否已初始化 */
    if (instance->state != OSAL_STATE_UNINITIALIZED) {
        return OSAL_ERROR_ALREADY_INIT;
    }

    /* 获取适配层描述（工厂模式） */
    const osal_port_desc_t* port = osal_get_port_desc(os_type);
    if (port == NULL) {
        return OSAL_ERROR_NOT_SUPPORTED;
    }

    /* 调用适配层初始化 */
    osal_status_t ret = port->init();
    if (ret != OSAL_OK) {
        return ret;
    }

    /* 初始化实例 */
    instance->state = OSAL_STATE_INITIALIZED;
    instance->os_type = os_type;
    instance->port = port;

    /* 调用用户Hook */
    osal_init_hook();

    return OSAL_OK;
}

osal_status_t osal_deinit(void) {
    osal_instance_t* instance = osal_get_instance();

    /* 检查是否已初始化 */
    if (instance->state == OSAL_STATE_UNINITIALIZED) {
        return OSAL_ERROR_NOT_INIT;
    }

    /* 调用适配层反初始化 */
    if (instance->port != NULL && instance->port->deinit != NULL) {
        instance->port->deinit();
    }

    /* 重置实例 */
    instance->state = OSAL_STATE_UNINITIALIZED;
    instance->os_type = OSAL_OS_NONE;
    instance->port = NULL;

    return OSAL_OK;
}

osal_status_t osal_start(void) {
    osal_instance_t* instance = osal_get_instance();

    /* 检查是否已初始化 */
    if (instance->state == OSAL_STATE_UNINITIALIZED) {
        return OSAL_ERROR_NOT_INIT;
    }

    /* 调用用户Hook */
    osal_start_hook();

    /* 更新状态 */
    instance->state = OSAL_STATE_RUNNING;

    /* 根据不同OS启动调度器 */
    switch (instance->os_type) {
#if OSAL_OS_TYPE == OSAL_OS_FREERTOS
        case OSAL_OS_FREERTOS:
            vTaskStartScheduler();
            break;

#elif OSAL_OS_TYPE == OSAL_OS_UCOS_II
        case OSAL_OS_UCOS_II:
            OSStart();
            break;

#elif OSAL_OS_TYPE == OSAL_OS_RTTHREAD
        case OSAL_OS_RTTHREAD:
            /* RT-Thread在rt_system_scheduler_init后自动启动 */
            break;
#endif
        default:
            return OSAL_ERROR_NOT_SUPPORTED;
    }

    return OSAL_OK;
}

osal_state_t osal_get_state(void) {
    osal_instance_t* instance = osal_get_instance();
    return instance->state;
}

osal_os_type_t osal_get_os_type(void) {
    osal_instance_t* instance = osal_get_instance();
    return instance->os_type;
}

const char* osal_get_version(void) { return OSAL_VERSION_STRING; }

int osal_is_in_isr(void) {
    osal_instance_t* instance = osal_get_instance();

    if (instance->port == NULL || instance->port->ops == NULL ||
        instance->port->ops->task == NULL) {
        return 0;
    }

    if (instance->port->ops->task->is_in_isr != NULL) {
        return instance->port->ops->task->is_in_isr();
    }

    return 0;
}

osal_status_t osal_scheduler_suspend(void) {
    osal_instance_t* instance = osal_get_instance();

    if (instance->port == NULL || instance->port->ops == NULL ||
        instance->port->ops->scheduler == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }

    return instance->port->ops->scheduler->suspend();
}

osal_status_t osal_scheduler_resume(void) {
    osal_instance_t* instance = osal_get_instance();

    if (instance->port == NULL || instance->port->ops == NULL ||
        instance->port->ops->scheduler == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }

    return instance->port->ops->scheduler->resume();
}

uint32_t osal_interrupt_disable(void) {
    /* 平台相关，需要根据具体平台实现 */
    return 0;
}

void osal_interrupt_restore(uint32_t state) {
    (void)state;
    /* 平台相关，需要根据具体平台实现 */
}

uint32_t osal_critical_enter(void) {
    /* 平台相关，需要根据具体平台实现 */
    return 0;
}

void osal_critical_exit(uint32_t state) {
    (void)state;
    /* 平台相关，需要根据具体平台实现 */
}

/* ==================== OSAL信息查询 ==================== */

osal_status_t osal_get_info(osal_info_t* info) {
#if OSAL_CFG_PARAM_CHECK
    if (info == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    osal_instance_t* instance = osal_get_instance();

    info->version = osal_get_version();
    info->os_type = instance->os_type;
    info->state = instance->state;
    info->tick_rate_hz = instance->tick_rate_hz;
    info->task_count = instance->task_count;
    info->queue_count = instance->queue_count;
    info->mutex_count = instance->mutex_count;
    info->semaphore_count = instance->semaphore_count;
    info->event_count = instance->event_count;

    return OSAL_OK;
}

/* ==================== 错误描述 ==================== */

const char* osal_strerror(osal_status_t status) {
    switch (status) {
        case OSAL_OK:
            return "OK";
        case OSAL_ERROR:
            return "Generic error";
        case OSAL_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case OSAL_ERROR_NO_MEM:
            return "Out of memory";
        case OSAL_ERROR_TIMEOUT:
            return "Timeout";
        case OSAL_ERROR_BUSY:
            return "Resource busy";
        case OSAL_ERROR_NOT_FOUND:
            return "Not found";
        case OSAL_ERROR_NOT_INIT:
            return "Not initialized";
        case OSAL_ERROR_ALREADY_INIT:
            return "Already initialized";
        case OSAL_ERROR_MAX_COUNT:
            return "Max count reached";
        case OSAL_ERROR_ISR:
            return "ISR context error";
        case OSAL_ERROR_NOT_SUPPORTED:
            return "Not supported";
        default:
            return "Unknown error";
    }
}

/* ==================== 时间相关 ==================== */

uint32_t osal_get_millis(void) {
    osal_instance_t* instance = osal_get_instance();

    if (instance->port == NULL || instance->port->ops == NULL ||
        instance->port->ops->task == NULL) {
        return 0;
    }

    if (instance->port->ops->task->get_tick_count != NULL) {
        osal_tick_t ticks = instance->port->ops->task->get_tick_count();
        return osal_ticks_to_ms(ticks);
    }

    return 0;
}

uint32_t osal_get_ticks(void) {
    osal_instance_t* instance = osal_get_instance();

    if (instance->port == NULL || instance->port->ops == NULL ||
        instance->port->ops->task == NULL) {
        return 0;
    }

    if (instance->port->ops->task->get_tick_count != NULL) {
        return instance->port->ops->task->get_tick_count();
    }

    return 0;
}

uint32_t osal_ticks_to_ms(uint32_t ticks) { return (ticks * 1000) / OSAL_CFG_TICK_RATE_HZ; }

uint32_t osal_ms_to_ticks(uint32_t ms) { return (ms * OSAL_CFG_TICK_RATE_HZ) / 1000; }

/* ==================== 调试和诊断 ==================== */

void osal_kernel_dump(void) {
#if OSAL_CFG_DEBUG_OUTPUT
    osal_info_t info;

    if (osal_get_info(&info) == OSAL_OK) {
        /* 打印OSAL信息 - 具体实现依赖于平台 */
        (void)info;
    }
#else
    (void)0; /* 避免未使用警告 */
#endif
}

/* ==================== Hook函数（弱定义，可重写） ==================== */

OSAL_WEAK void osal_init_hook(void) { /* 用户可重写 */ }

OSAL_WEAK void osal_start_hook(void) { /* 用户可重写 */ }

OSAL_WEAK void osal_idle_hook(void) { /* 用户可重写 */ }

OSAL_WEAK void osal_tick_hook(void) { /* 用户可重写 */ }

OSAL_WEAK void osal_stack_overflow_hook(osal_task_t task_handle, const char* task_name) {
    (void)task_handle;
    (void)task_name;
    /* 用户可重写 */
}

OSAL_WEAK void osal_malloc_failed_hook(uint32_t size) {
    (void)size;
    /* 用户可重写 */
}
