/**
 * @file osal_ops.h
 * @brief OSAL操作接口结构体定义
 * @details 定义策略模式使用的函数指针结构体
 */

#ifndef OSAL_OPS_H
#define OSAL_OPS_H

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OSAL任务操作接口结构体
 * @details 采用策略模式，每个RTOS实现自己的操作接口
 */
typedef struct {
    osal_status_t (*create)(const osal_task_config_t* config, osal_task_t* handle);
    osal_status_t (*delete)(osal_task_t handle);
    osal_status_t (*suspend)(osal_task_t handle);
    osal_status_t (*resume)(osal_task_t handle);
    osal_task_t (*get_current)(void);
    osal_status_t (*yield)(void);
    osal_status_t (*delay)(osal_tick_t ticks);
    osal_status_t (*delay_until)(osal_tick_t* prev_tick, osal_tick_t ticks);
    osal_status_t (*get_priority)(osal_task_t handle, osal_priority_t* priority);
    osal_status_t (*set_priority)(osal_task_t handle, osal_priority_t priority);
    osal_status_t (*get_state)(osal_task_t handle, osal_task_state_t* state);
    osal_status_t (*get_info)(osal_task_t handle, const char** name, uint32_t* stack_size,
                              uint32_t* stack_free);
    osal_tick_t (*get_tick_count)(void);
    int (*is_in_isr)(void);
} osal_task_ops_t;

/**
 * @brief OSAL队列操作接口结构体
 */
typedef struct {
    osal_status_t (*create)(const osal_queue_config_t* config, osal_queue_t* handle);
    osal_status_t (*delete)(osal_queue_t handle);
    osal_status_t (*send)(osal_queue_t handle, const void* data, osal_tick_t timeout);
    osal_status_t (*send_front)(osal_queue_t handle, const void* data, osal_tick_t timeout);
    osal_status_t (*receive)(osal_queue_t handle, void* data, osal_tick_t timeout);
    osal_status_t (*reset)(osal_queue_t handle);
    osal_status_t (*get_count)(osal_queue_t handle, uint32_t* count);
    osal_status_t (*get_space)(osal_queue_t handle, uint32_t* space);
    osal_status_t (*is_empty)(osal_queue_t handle, int* is_empty);
    osal_status_t (*is_full)(osal_queue_t handle, int* is_full);
    osal_status_t (*get_info)(osal_queue_t handle, const char** name, uint32_t* max_items,
                              uint32_t* item_size);
} osal_queue_ops_t;

/**
 * @brief OSAL互斥锁操作接口结构体
 */
typedef struct {
    osal_status_t (*create)(const osal_mutex_config_t* config, osal_mutex_t* handle);
    osal_status_t (*delete)(osal_mutex_t handle);
    osal_status_t (*acquire)(osal_mutex_t handle, osal_tick_t timeout);
    osal_status_t (*release)(osal_mutex_t handle);
    osal_status_t (*get_owner)(osal_mutex_t handle, osal_task_t* owner);
    osal_status_t (*get_info)(osal_mutex_t handle, const char** name, uint8_t* inherit);
} osal_mutex_ops_t;

/**
 * @brief OSAL信号量操作接口结构体
 */
typedef struct {
    osal_status_t (*create)(const osal_semaphore_config_t* config, osal_semaphore_t* handle);
    osal_status_t (*delete)(osal_semaphore_t handle);
    osal_status_t (*acquire)(osal_semaphore_t handle, osal_tick_t timeout);
    osal_status_t (*release)(osal_semaphore_t handle);
    osal_status_t (*get_count)(osal_semaphore_t handle, uint32_t* count);
    osal_status_t (*set_count)(osal_semaphore_t handle, uint32_t count);
    osal_status_t (*get_info)(osal_semaphore_t handle, const char** name,
                              osal_semaphore_type_t* type, uint32_t* max_count);
} osal_semaphore_ops_t;

/**
 * @brief OSAL事件操作接口结构体
 */
typedef struct {
    osal_status_t (*create)(const osal_event_config_t* config, osal_event_t* handle);
    osal_status_t (*delete)(osal_event_t handle);
    osal_status_t (*wait)(osal_event_t handle, osal_event_flags_t wait_flags, uint8_t option,
                          osal_event_flags_t* actual_flags, osal_tick_t timeout);
    osal_status_t (*set)(osal_event_t handle, osal_event_flags_t flags);
    osal_status_t (*clear)(osal_event_t handle, osal_event_flags_t flags);
    osal_status_t (*get)(osal_event_t handle, osal_event_flags_t* flags);
    osal_status_t (*sync)(osal_event_t handle, osal_event_flags_t flags);
    osal_status_t (*get_info)(osal_event_t handle, const char** name);
} osal_event_ops_t;

/**
 * @brief OSAL任务通知操作接口结构体
 */
typedef struct {
    osal_status_t (*notify)(osal_task_t task, uint32_t value, uint8_t action, uint32_t* prev_value);
    osal_status_t (*wait)(uint32_t clear_bits_entry, uint32_t clear_bits_exit, uint32_t* value,
                          osal_tick_t timeout);
    osal_status_t (*clear)(osal_task_t task, uint32_t bits_to_clear, uint32_t* prev_value);
} osal_task_notify_ops_t;

/**
 * @brief OSAL内存管理操作接口结构体
 */
typedef struct {
    void* (*alloc)(uint32_t size);
    osal_status_t (*free)(void* ptr);
    void* (*realloc)(void* ptr, uint32_t size);
    void* (*calloc)(uint32_t num, uint32_t size);
    uint32_t (*get_free_size)(void);
    uint32_t (*get_minimum_free_size)(void);
} osal_memory_ops_t;

/**
 * @brief OSAL链表操作接口结构体
 */
typedef struct {
    osal_status_t (*create)(const osal_list_config_t* config, osal_list_t* handle);
    osal_status_t (*del)(osal_list_t handle);
    osal_status_t (*push_front)(osal_list_t handle, const void* data);
    osal_status_t (*push_back)(osal_list_t handle, const void* data);
    osal_status_t (*insert_after)(osal_list_t handle, osal_list_node_t* position, const void* data);
    osal_status_t (*insert_before)(osal_list_t handle, osal_list_node_t* position,
                                   const void* data);
    osal_status_t (*pop_front)(osal_list_t handle, void* data);
    osal_status_t (*pop_back)(osal_list_t handle, void* data);
    osal_status_t (*remove)(osal_list_t handle, osal_list_node_t* node, void* data);
    osal_status_t (*front)(osal_list_t handle, osal_list_node_t** node);
    osal_status_t (*back)(osal_list_t handle, osal_list_node_t** node);
    osal_status_t (*next)(osal_list_node_t* node, osal_list_node_t** next);
    osal_status_t (*prev)(osal_list_node_t* node, osal_list_node_t** prev);
    osal_status_t (*get_data)(osal_list_node_t* node, void** data);
    osal_status_t (*set_data)(osal_list_node_t* node, const void* data);
    osal_status_t (*get_count)(osal_list_t handle, uint32_t* count);
    osal_status_t (*is_empty)(osal_list_t handle, int* is_empty);
    osal_status_t (*clear)(osal_list_t handle);
    osal_status_t (*reverse)(osal_list_t handle);
    osal_status_t (*sort)(osal_list_t handle, osal_list_compare_cb_t compare);
    osal_status_t (*find)(osal_list_t handle, const void* data, osal_list_compare_cb_t compare,
                          osal_list_node_t** node);
    osal_status_t (*traverse)(osal_list_t handle, osal_list_traverse_cb_t callback, void* context);
    osal_status_t (*get_info)(osal_list_t handle, const char** name, uint32_t* count,
                              uint32_t* max_nodes);
    osal_status_t (*at)(osal_list_t handle, uint32_t index, osal_list_node_t** node);
} osal_list_ops_t;

/**
 * @brief OSAL调度器控制操作接口结构体
 */
typedef struct {
    osal_status_t (*suspend)(void);
    osal_status_t (*resume)(void);
} osal_scheduler_ops_t;

/**
 * @brief OSAL完整操作接口集合
 * @details 将所有组件的操作接口集合在一起，形成完整的RTOS适配层接口
 */
typedef struct {
    const osal_task_ops_t* task;               /**< 任务操作接口 */
    const osal_queue_ops_t* queue;             /**< 队列操作接口 */
    const osal_mutex_ops_t* mutex;             /**< 互斥锁操作接口 */
    const osal_semaphore_ops_t* semaphore;     /**< 信号量操作接口 */
    const osal_event_ops_t* event;             /**< 事件操作接口 */
    const osal_task_notify_ops_t* task_notify; /**< 任务通知操作接口 */
    const osal_memory_ops_t* memory;           /**< 内存管理操作接口 */
    const osal_scheduler_ops_t* scheduler;     /**< 调度器控制接口 */
    const osal_list_ops_t* list;               /**< 链表操作接口 */
} osal_ops_t;

/**
 * @brief OSAL适配层初始化/反初始化函数类型
 */
typedef osal_status_t (*osal_port_init_fn_t)(void);
typedef osal_status_t (*osal_port_deinit_fn_t)(void);

/**
 * @brief OSAL适配层描述结构体
 * @details 工厂模式使用的适配层描述
 */
typedef struct {
    osal_os_type_t os_type;       /**< OS类型 */
    const char* name;             /**< 适配层名称 */
    const osal_ops_t* ops;        /**< 操作接口集合 */
    osal_port_init_fn_t init;     /**< 初始化函数 */
    osal_port_deinit_fn_t deinit; /**< 反初始化函数 */
} osal_port_desc_t;

/**
 * @brief OSAL实例结构体（单例模式）
 */
typedef struct {
    osal_state_t state;           /**< OSAL状态 */
    osal_os_type_t os_type;       /**< 当前OS类型 */
    const osal_port_desc_t* port; /**< 当前使用的适配层 */
    uint32_t task_count;          /**< 任务计数 */
    uint32_t queue_count;         /**< 队列计数 */
    uint32_t mutex_count;         /**< 互斥锁计数 */
    uint32_t semaphore_count;     /**< 信号量计数 */
    uint32_t event_count;         /**< 事件计数 */
    uint32_t task_notify_count;   /**< 任务通知计数 */
    uint32_t tick_rate_hz;        /**< 系统时钟频率 */
} osal_instance_t;

/**
 * @brief 获取OSAL操作接口（内部使用）
 * @return 操作接口指针，NULL表示未初始化
 */
const osal_ops_t* osal_get_ops(void);

/**
 * @brief 获取OSAL适配层描述（内部使用）
 * @return 适配层描述指针，NULL表示未初始化
 */
const osal_port_desc_t* osal_get_current_port(void);

/**
 * @brief 获取OSAL实例（内部使用）
 * @return OSAL实例指针
 */
osal_instance_t* osal_get_instance_internal(void);

/**
 * @brief 获取指定OS类型的适配层描述
 * @param os_type OS类型
 * @return 适配层描述指针，NULL表示不支持
 */
const osal_port_desc_t* osal_get_port_desc(osal_os_type_t os_type);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_OPS_H */
