/**
 * @file osal_config.h
 * @brief OSAL配置文件
 * @details 配置OSAL的各项参数和功能开关
 */

#ifndef OSAL_CONFIG_H
#define OSAL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== OS类型选择 ==================== */

/**
 * @brief OS类型枚举
 */
typedef enum {
    OSAL_OS_NONE = 0,     /**< 无OS（裸机） */
    OSAL_OS_FREERTOS = 1, /**< FreeRTOS */
    OSAL_OS_UCOS_II = 2,  /**< uC/OS-II */
    OSAL_OS_RTTHREAD = 3  /**< RT-Thread */
} osal_os_type_t;

/**
 * @brief 当前使用的OS类型（编译时配置）
 * @note 可以在编译时通过宏定义指定，如 -DOSAL_OS_TYPE=OSAL_OS_FREERTOS
 */
#ifndef OSAL_OS_TYPE
#define OSAL_OS_TYPE OSAL_OS_FREERTOS
#endif

/* ==================== 功能模块开关 ==================== */

/** @brief 是否启用任务模块 */
#ifndef OSAL_CFG_TASK_ENABLE
#define OSAL_CFG_TASK_ENABLE 1
#endif

/** @brief 是否启用队列模块 */
#ifndef OSAL_CFG_QUEUE_ENABLE
#define OSAL_CFG_QUEUE_ENABLE 1
#endif

/** @brief 是否启用互斥锁模块 */
#ifndef OSAL_CFG_MUTEX_ENABLE
#define OSAL_CFG_MUTEX_ENABLE 1
#endif

/** @brief 是否启用信号量模块 */
#ifndef OSAL_CFG_SEMAPHORE_ENABLE
#define OSAL_CFG_SEMAPHORE_ENABLE 1
#endif

/** @brief 是否启用事件模块 */
#ifndef OSAL_CFG_EVENT_ENABLE
#define OSAL_CFG_EVENT_ENABLE 1
#endif

/** @brief 是否启用内存管理模块 */
#ifndef OSAL_CFG_MEMORY_ENABLE
#define OSAL_CFG_MEMORY_ENABLE 1
#endif

/* ==================== 参数配置 ==================== */

/** @brief 最大任务名称长度 */
#ifndef OSAL_CFG_MAX_TASK_NAME_LEN
#define OSAL_CFG_MAX_TASK_NAME_LEN 16
#endif

/** @brief 默认任务栈大小（字节） */
#ifndef OSAL_CFG_DEFAULT_STACK_SIZE
#define OSAL_CFG_DEFAULT_STACK_SIZE 512
#endif

/** @brief 默认任务优先级 */
#ifndef OSAL_CFG_DEFAULT_PRIORITY
#define OSAL_CFG_DEFAULT_PRIORITY OSAL_PRIORITY_NORMAL
#endif

/** @brief 最大任务数量 */
#ifndef OSAL_CFG_MAX_TASKS
#define OSAL_CFG_MAX_TASKS 16
#endif

/** @brief 最大队列数量 */
#ifndef OSAL_CFG_MAX_QUEUES
#define OSAL_CFG_MAX_QUEUES 16
#endif

/** @brief 最大互斥锁数量 */
#ifndef OSAL_CFG_MAX_MUTEXES
#define OSAL_CFG_MAX_MUTEXES 16
#endif

/** @brief 最大信号量数量 */
#ifndef OSAL_CFG_MAX_SEMAPHORES
#define OSAL_CFG_MAX_SEMAPHORES 16
#endif

/** @brief 最大事件标志组数量 */
#ifndef OSAL_CFG_MAX_EVENTS
#define OSAL_CFG_MAX_EVENTS 16
#endif

/* ==================== 调试配置 ==================== */

/** @brief 是否启用参数检查 */
#ifndef OSAL_CFG_PARAM_CHECK
#define OSAL_CFG_PARAM_CHECK 1
#endif

/** @brief 是否启用调试输出 */
#ifndef OSAL_CFG_DEBUG_OUTPUT
#define OSAL_CFG_DEBUG_OUTPUT 0
#endif

/** @brief 是否启用统计信息 */
#ifndef OSAL_CFG_STATISTICS_ENABLE
#define OSAL_CFG_STATISTICS_ENABLE 0
#endif

/* ==================== 内存管理配置 ==================== */

/** @brief 内存对齐（字节） */
#ifndef OSAL_CFG_MEMORY_ALIGNMENT
#define OSAL_CFG_MEMORY_ALIGNMENT 8
#endif

/** @brief 是否使用内存池管理 */
#ifndef OSAL_CFG_USE_MEMORY_POOL
#define OSAL_CFG_USE_MEMORY_POOL 0
#endif

/* ==================== API实现配置 ==================== */

/**
 * @brief API实现方式
 * 0: 静态函数调用（编译时确定）
 * 1: 函数指针表（运行时切换）
 */
#ifndef OSAL_CFG_USE_FUNCTION_PTR
#define OSAL_CFG_USE_FUNCTION_PTR 1
#endif

/* ==================== Hook函数配置 ==================== */

/** @brief 是否启用任务切换Hook */
#ifndef OSAL_CFG_HOOK_TASK_SWITCH
#define OSAL_CFG_HOOK_TASK_SWITCH 0
#endif

/** @brief 是否启用空闲任务Hook */
#ifndef OSAL_CFG_HOOK_IDLE
#define OSAL_CFG_HOOK_IDLE 0
#endif

/** @brief 是否启用内存分配失败Hook */
#ifndef OSAL_CFG_HOOK_MALLOC_FAILED
#define OSAL_CFG_HOOK_MALLOC_FAILED 1
#endif

/** @brief 是否启用栈溢出Hook */
#ifndef OSAL_CFG_HOOK_STACK_OVERFLOW
#define OSAL_CFG_HOOK_STACK_OVERFLOW 1
#endif

/* ==================== 时间配置 ==================== */

/** @brief 系统时钟频率（Hz） */
#ifndef OSAL_CFG_TICK_RATE_HZ
#define OSAL_CFG_TICK_RATE_HZ 1000
#endif

/* ==================== ISR安全配置 ==================== */

/** @brief 是否在ISR中检测API调用 */
#ifndef OSAL_CFG_ISR_SAFE_API
#define OSAL_CFG_ISR_SAFE_API 1
#endif

#if OSAL_CFG_ISR_SAFE_API
#define OSAL_API_FROM_ISR osal_api_from_isr()
#else
#define OSAL_API_FROM_ISR 0
#endif

/* ==================== 版本信息 ==================== */

#define OSAL_VERSION_MAJOR 1
#define OSAL_VERSION_MINOR 0
#define OSAL_VERSION_PATCH 0
#define OSAL_VERSION_STRING "1.0.0"

/* ==================== 兼容性配置 ==================== */

/** @brief 兼容性模式：严格/宽松 */
#ifndef OSAL_CFG_STRICT_MODE
#define OSAL_CFG_STRICT_MODE 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CONFIG_H */
