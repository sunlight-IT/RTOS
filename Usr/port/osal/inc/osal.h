/**
 * @file osal.h
 * @brief OSAL主头文件
 * @details 操作系统抽象层(OSAL)主入口，统一管理所有组件
 */

#ifndef OSAL_H
#define OSAL_H

#include "osal_config.h"
#include "osal_types.h"

/* 引入所有组件头文件 */
#include "osal_event.h"
#include "osal_list.h"
#include "osal_memory.h"
#include "osal_mutex.h"
#include "osal_queue.h"
#include "osal_semaphore.h"
#include "osal_task.h"
#include "osal_task_notify.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== OSAL状态 ==================== */

/**
 * @brief OSAL初始化状态枚举
 */
typedef enum {
    OSAL_STATE_UNINITIALIZED = 0, /**< 未初始化 */
    OSAL_STATE_INITIALIZED = 1,   /**< 已初始化 */
    OSAL_STATE_RUNNING = 2,       /**< 运行中 */
    OSAL_STATE_ERROR = 3          /**< 错误状态 */
} osal_state_t;

/* ==================== OSAL初始化和控制接口 ==================== */

/**
 * @brief 初始化OSAL
 * @details 初始化OSAL系统，必须在调用其他OSAL函数之前调用
 * @param os_type 使用的操作系统类型
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_ALREADY_INIT 已经初始化
 *         OSAL_ERROR_NOT_INIT 初始化失败
 */
osal_status_t osal_init(osal_os_type_t os_type);

/**
 * @brief 反初始化OSAL
 * @details 清理OSAL系统资源
 * @return OSAL_OK 成功
 *         OSAL_ERROR_NOT_INIT 未初始化
 */
osal_status_t osal_deinit(void);

/**
 * @brief 启动OSAL调度器
 * @details 启动任务调度，正常情况下不会返回
 * @return OSAL_OK 成功启动调度
 *         OSAL_ERROR_NOT_INIT 未初始化
 */
osal_status_t osal_start(void);

/**
 * @brief 获取OSAL状态
 * @return OSAL状态
 */
osal_state_t osal_get_state(void);

/**
 * @brief 获取当前OS类型
 * @return 当前OS类型
 */
osal_os_type_t osal_get_os_type(void);

/**
 * @brief 获取OSAL版本信息
 * @return 版本字符串
 */
const char* osal_get_version(void);

/**
 * @brief 检查是否在ISR上下文中
 * @return 1 在ISR中，0 不在ISR中
 */
int osal_is_in_isr(void);

/**
 * @brief 暂停调度器
 * @details 暂停任务调度，用于临界区保护
 * @return OSAL_OK 成功
 */
osal_status_t osal_scheduler_suspend(void);

/**
 * @brief 恢复调度器
 * @details 恢复任务调度
 * @return OSAL_OK 成功
 */
osal_status_t osal_scheduler_resume(void);

/**
 * @brief 禁用中断
 * @return 中断禁用前的状态（用于恢复）
 */
uint32_t osal_interrupt_disable(void);

/**
 * @brief 恢复中断
 * @param state 中断状态（由interrupt_disable返回）
 */
void osal_interrupt_restore(uint32_t state);

/**
 * @brief 进入临界区
 * @return 临界区状态（用于退出）
 */
uint32_t osal_critical_enter(void);

/**
 * @brief 退出临界区
 * @param state 临界区状态（由critical_enter返回）
 */
void osal_critical_exit(uint32_t state);

/* ==================== OSAL信息查询 ==================== */

/**
 * @brief OSAL系统信息结构体
 */
typedef struct {
    const char* version;        /**< 版本号 */
    osal_os_type_t os_type;     /**< OS类型 */
    osal_state_t state;         /**< OSAL状态 */
    uint32_t tick_rate_hz;      /**< 系统时钟频率 */
    uint32_t task_count;        /**< 当前任务数量 */
    uint32_t queue_count;       /**< 当前队列数量 */
    uint32_t mutex_count;       /**< 当前互斥锁数量 */
    uint32_t semaphore_count;   /**< 当前信号量数量 */
    uint32_t event_count;       /**< 当前事件数量 */
    uint32_t task_notify_count; /**< 当前任务通知数量 */
} osal_info_t;

/**
 * @brief 获取OSAL系统信息
 * @param[out] info 信息结构体指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_get_info(osal_info_t* info);

/* ==================== OSAL Hook函数 ==================== */

/**
 * @brief OSAL初始化完成Hook函数
 * @details 用户可重写此函数进行自定义初始化
 */
OSAL_WEAK void osal_init_hook(void);

/**
 * @brief OSAL启动前Hook函数
 * @details 用户可重写此函数在调度器启动前执行
 */
OSAL_WEAK void osal_start_hook(void);

/**
 * @brief OSAL空闲Hook函数
 * @details 用户可重写此函数在空闲时执行
 */
OSAL_WEAK void osal_idle_hook(void);

/**
 * @brief OSAL滴答Hook函数
 * @details 每次系统滴答调用
 */
OSAL_WEAK void osal_tick_hook(void);

/**
 * @brief 栈溢出Hook函数
 * @param task_handle 发生栈溢出的任务句柄
 * @param task_name 任务名称
 */
OSAL_WEAK void osal_stack_overflow_hook(osal_task_t task_handle, const char* task_name);

/**
 * @brief 内存分配失败Hook函数
 * @param size 请求分配的大小
 */
OSAL_WEAK void osal_malloc_failed_hook(uint32_t size);

/* ==================== 调试和诊断 ==================== */

/**
 * @brief OSAL内核状态转储
 * @details 打印OSAL系统状态信息（需要OSAL_CFG_DEBUG_OUTPUT启用）
 */
void osal_kernel_dump(void);

/**
 * @brief 获取错误描述字符串
 * @param status 状态码
 * @return 错误描述字符串
 */
const char* osal_strerror(osal_status_t status);

/* ==================== 时间相关 ==================== */

/**
 * @brief 获取系统运行时间(ms)
 * @return 系统运行时间(ms)
 */
uint32_t osal_get_millis(void);

/**
 * @brief 获取系统tick数
 * @return 系统tick数
 */
uint32_t osal_get_ticks(void);

/**
 * @brief tick转换为毫秒
 * @param ticks tick数
 * @return 毫秒数
 */
uint32_t osal_ticks_to_ms(uint32_t ticks);

/**
 * @brief 毫秒转换为tick
 * @param ms 毫秒数
 * @return tick数
 */
uint32_t osal_ms_to_ticks(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */
