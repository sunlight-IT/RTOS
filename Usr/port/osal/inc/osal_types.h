/**
 * @file osal_types.h
 * @brief OSAL类型定义和错误码
 * @details 定义OSAL使用的所有基础类型、错误码和宏定义
 */

#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 类型定义 ==================== */

/**
 * @brief OSAL句柄类型（通用句柄）
 */
typedef void* osal_handle_t;

/**
 * @brief 任务句柄
 */
typedef void* osal_task_t;

/**
 * @brief 队列句柄
 */
typedef void* osal_queue_t;

/**
 * @brief 互斥锁句柄
 */
typedef void* osal_mutex_t;

/**
 * @brief 信号量句柄
 */
typedef void* osal_semaphore_t;

/**
 * @brief 事件句柄
 */
typedef void* osal_event_t;

/**
 * @brief 内存池句柄
 */
typedef void* osal_memory_t;

/**
 * @brief 任务通知句柄
 */
typedef void* osal_task_notify_t;

/**
 * @brief 链表句柄
 */
typedef void* osal_list_t;

/**
 * @brief 链表节点句柄
 */
typedef void osal_list_node_t;

/**
 * @brief 任务函数指针类型
 * @param param 任务参数
 */
typedef void (*osal_task_func_t)(void* param);

/**
 * @brief 任务优先级类型
 */
typedef uint8_t osal_priority_t;

/**
 * @brief 时间类型（毫秒）
 */
typedef uint32_t osal_tick_t;

/* ==================== 错误码定义 ==================== */

/**
 * @brief OSAL状态码枚举
 */
typedef enum {
    OSAL_OK = 0,                   /**< 操作成功 */
    OSAL_ERROR = -1,               /**< 通用错误 */
    OSAL_ERROR_INVALID_PARAM = -2, /**< 无效参数 */
    OSAL_ERROR_NO_MEM = -3,        /**< 内存不足 */
    OSAL_ERROR_TIMEOUT = -4,       /**< 操作超时 */
    OSAL_ERROR_BUSY = -5,          /**< 资源忙 */
    OSAL_ERROR_NOT_FOUND = -6,     /**< 资源未找到 */
    OSAL_ERROR_NOT_INIT = -7,      /**< 未初始化 */
    OSAL_ERROR_ALREADY_INIT = -8,  /**< 已经初始化 */
    OSAL_ERROR_MAX_COUNT = -9,     /**< 达到最大数量 */
    OSAL_ERROR_ISR = -10,          /**< ISR上下文错误 */
    OSAL_ERROR_NOT_SUPPORTED = -11 /**< 功能不支持 */
} osal_status_t;

/* ==================== 任务优先级宏定义 ==================== */

#define OSAL_PRIORITY_IDLE 0          /**< 空闲任务优先级 */
#define OSAL_PRIORITY_LOW 5           /**< 低优先级 */
#define OSAL_PRIORITY_NORMAL 10       /**< 普通优先级 */
#define OSAL_PRIORITY_ABOVE_NORMAL 15 /**< 高于普通优先级 */
#define OSAL_PRIORITY_HIGH 20         /**< 高优先级 */
#define OSAL_PRIORITY_REALTIME 25     /**< 实时优先级 */
#define OSAL_PRIORITY_MAX 31          /**< 最大优先级 */

/* ==================== 时间宏定义 ==================== */

#define OSAL_WAIT_FOREVER 0xFFFFFFFF /**< 永久等待 */
#define OSAL_NO_WAIT 0               /**< 不等待 */

/* ==================== 任务状态定义 ==================== */

/**
 * @brief 任务状态枚举
 */
typedef enum {
    OSAL_TASK_STATE_READY = 0,     /**< 就绪状态 */
    OSAL_TASK_STATE_RUNNING = 1,   /**< 运行状态 */
    OSAL_TASK_STATE_BLOCKED = 2,   /**< 阻塞状态 */
    OSAL_TASK_STATE_SUSPENDED = 3, /**< 挂起状态 */
    OSAL_TASK_STATE_DELETED = 4    /**< 删除状态 */
} osal_task_state_t;

/* ==================== 任务配置结构体 ==================== */

/**
 * @brief 任务配置结构体
 */
typedef struct {
    const char* name;         /**< 任务名称 */
    osal_task_func_t func;    /**< 任务函数 */
    void* param;              /**< 任务参数 */
    uint32_t stack_size;      /**< 栈大小(字节) */
    osal_priority_t priority; /**< 任务优先级 */
    uint32_t time_slice;      /**< 时间片(0为默认) */
} osal_task_config_t;

/* ==================== 事件标志定义 ==================== */

/**
 * @brief 事件标志类型（32位）
 */
typedef uint32_t osal_event_flags_t;

/* 事件等待选项 */
#define OSAL_EVENT_WAIT_ANY 0x00   /**< 等待任意标志 */
#define OSAL_EVENT_WAIT_ALL 0x01   /**< 等待所有标志 */
#define OSAL_EVENT_WAIT_CLEAR 0x02 /**< 等待后清除标志 */

/* ==================== 内存管理宏 ==================== */

/**
 * @brief 内存对齐宏
 */
#define OSAL_ALIGN(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

#define OSAL_ALIGN_4BYTE(size) OSAL_ALIGN(size, 4)
#define OSAL_ALIGN_8BYTE(size) OSAL_ALIGN(size, 8)

/* ==================== 工具宏 ==================== */

/**
 * @brief 获取数组元素个数
 */
#define OSAL_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief 最小值宏
 */
#define OSAL_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief 最大值宏
 */
#define OSAL_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief 限制范围宏
 */
#define OSAL_CLAMP(val, min, max) (OSAL_MIN(OSAL_MAX(val, min), max))

/* ==================== 调试相关宏 ==================== */

#ifndef OSAL_ASSERT
#define OSAL_ASSERT(expr) \
    do {                  \
        if (!(expr)) {    \
            while (1);    \
        }                 \
    } while (0)
#endif

/* ==================== 属性宏 ==================== */

/**
 * @brief 打包属性（用于结构体对齐）
 */
#if defined(__GNUC__)
#define OSAL_PACKED __attribute__((packed))
#elif defined(__CC_ARM)
#define OSAL_PACKED __packed
#else
#define OSAL_PACKED
#endif

/**
 * @brief 对齐属性
 */
#if defined(__GNUC__)
#define OSAL_ALIGN_ATTR(x) __attribute__((aligned(x)))
#else
#define OSAL_ALIGN_ATTR(x)
#endif

/**
 * @brief 弱符号属性
 */
#if defined(__GNUC__)
#define OSAL_WEAK __attribute__((weak))
#elif defined(__CC_ARM)
#define OSAL_WEAK __weak
#else
#define OSAL_WEAK
#endif

/**
 * @brief 内联函数属性
 */
#if defined(__GNUC__)
#define OSAL_INLINE static inline __attribute__((always_inline))
#elif defined(__CC_ARM)
#define OSAL_INLINE static __inline
#else
#define OSAL_INLINE static inline
#endif

#ifdef __cplusplus
}
#endif

#endif /* OSAL_TYPES_H */
