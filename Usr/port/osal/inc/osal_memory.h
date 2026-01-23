/**
 * @file osal_memory.h
 * @brief OSAL内存管理组件接口
 * @details 提供动态内存分配、释放等功能，支持内存池管理
 */

#ifndef OSAL_MEMORY_H
#define OSAL_MEMORY_H

#include "osal_config.h"
#include "osal_types.h"

#if OSAL_CFG_MEMORY_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 内存配置结构体 ==================== */

/**
 * @brief 内存池配置结构体
 */
typedef struct {
    const char* name;    /**< 内存池名称 */
    void* addr;          /**< 内存池起始地址 */
    uint32_t size;       /**< 内存池总大小(字节) */
    uint32_t block_size; /**< 每个块大小(字节) */
} osal_memory_pool_config_t;

/* ==================== 内存操作接口 ==================== */

/**
 * @brief 分配内存
 * @param size 分配的大小(字节)
 * @return 分配的内存指针，NULL表示失败
 */
void* osal_memory_alloc(uint32_t size);

/**
 * @brief 释放内存
 * @param ptr 要释放的内存指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_memory_free(void* ptr);

/**
 * @brief 重新分配内存
 * @param ptr 原内存指针
 * @param size 新的大小(字节)
 * @return 重新分配后的内存指针，NULL表示失败
 */
void* osal_memory_realloc(void* ptr, uint32_t size);

/**
 * @brief 分配对齐的内存
 * @param size 分配的大小(字节)
 * @param alignment 对齐字节数（必须是2的幂）
 * @return 分配的内存指针，NULL表示失败
 */
void* osal_memory_alloc_aligned(uint32_t size, uint32_t alignment);

/**
 * @brief 分配并清零内存
 * @param num 元素个数
 * @param size 每个元素的大小(字节)
 * @return 分配的内存指针，NULL表示失败
 */
void* osal_memory_calloc(uint32_t num, uint32_t size);

/**
 * @brief 复制内存
 * @param dest 目标地址
 * @param src 源地址
 * @param size 复制大小(字节)
 * @return 目标地址
 */
void* osal_memory_copy(void* dest, const void* src, uint32_t size);

/**
 * @brief 设置内存
 * @param ptr 内存地址
 * @param value 设置的值
 * @param size 设置大小(字节)
 * @return 内存地址
 */
void* osal_memory_set(void* ptr, int value, uint32_t size);

/**
 * @brief 移动内存（处理重叠区域）
 * @param dest 目标地址
 * @param src 源地址
 * @param size 移动大小(字节)
 * @return 目标地址
 */
void* osal_memory_move(void* dest, const void* src, uint32_t size);

/**
 * @brief 比较内存
 * @param ptr1 内存地址1
 * @param ptr2 内存地址2
 * @param size 比较大小(字节)
 * @return 0 相等，<0 ptr1<ptr2，>0 ptr1>ptr2
 */
int osal_memory_compare(const void* ptr1, const void* ptr2, uint32_t size);

/**
 * @brief 获取内存大小
 * @param ptr 内存指针
 * @param[out] size 大小输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_memory_get_size(const void* ptr, uint32_t* size);

/**
 * @brief 获取可用内存大小
 * @return 可用内存大小(字节)
 */
uint32_t osal_memory_get_free_size(void);

/**
 * @brief 获取最小可用内存块大小
 * @return 最小可用内存块大小(字节)
 */
uint32_t osal_memory_get_minimum_free_size(void);

/* ==================== 内存池操作接口 ==================== */

/**
 * @brief 创建内存池
 * @param config 内存池配置结构体指针
 * @param[out] handle 内存池句柄输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_NO_MEM 内存不足
 */
osal_status_t osal_memory_pool_create(const osal_memory_pool_config_t* config,
                                      osal_memory_t* handle);

/**
 * @brief 删除内存池
 * @param handle 内存池句柄
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 *         OSAL_ERROR_BUSY 内存池正在被使用
 */
osal_status_t osal_memory_pool_delete(osal_memory_t handle);

/**
 * @brief 从内存池分配内存
 * @param handle 内存池句柄
 * @param timeout 超时时间(ms)
 * @return 分配的内存指针，NULL表示失败或超时
 */
void* osal_memory_pool_alloc(osal_memory_t handle, osal_tick_t timeout);

/**
 * @brief 释放内存到内存池
 * @param handle 内存池句柄
 * @param ptr 要释放的内存指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_memory_pool_free(osal_memory_t handle, void* ptr);

/**
 * @brief 获取内存池可用块数量
 * @param handle 内存池句柄
 * @param[out] count 可用块数量输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_memory_pool_get_available_blocks(osal_memory_t handle, uint32_t* count);

/**
 * @brief 获取内存池信息
 * @param handle 内存池句柄
 * @param[out] name 内存池名称输出指针（可为NULL）
 * @param[out] total_size 总大小输出指针（可为NULL）
 * @param[out] block_size 块大小输出指针（可为NULL）
 * @param[out] free_blocks 空闲块数量输出指针（可为NULL）
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_memory_pool_get_info(osal_memory_t handle, const char** name,
                                        uint32_t* total_size, uint32_t* block_size,
                                        uint32_t* free_blocks);

/* ==================== 内存统计信息 ==================== */

/**
 * @brief 内存使用统计结构体
 */
typedef struct {
    uint32_t total_size;    /**< 总内存大小 */
    uint32_t used_size;     /**< 已使用大小 */
    uint32_t free_size;     /**< 空闲大小 */
    uint32_t min_free_size; /**< 历史最小空闲大小 */
    uint32_t alloc_count;   /**< 分配次数 */
    uint32_t free_count;    /**< 释放次数 */
} osal_memory_stats_t;

/**
 * @brief 获取内存统计信息
 * @param[out] stats 统计信息输出指针
 * @return OSAL_OK 成功
 *         OSAL_ERROR_INVALID_PARAM 参数错误
 */
osal_status_t osal_memory_get_stats(osal_memory_stats_t* stats);

/* ==================== Hook函数 ==================== */

/**
 * @brief 内存分配失败Hook函数
 */
OSAL_WEAK void osal_memory_alloc_failed_hook(uint32_t size);

/**
 * @brief 内存检测Hook函数（用于检测内存溢出等）
 */
OSAL_WEAK void osal_memory_check_hook(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_MEMORY_ENABLE */

#endif /* OSAL_MEMORY_H */
