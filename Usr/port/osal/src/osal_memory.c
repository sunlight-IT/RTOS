/**
 * @file osal_memory.c
 * @brief OSAL内存管理组件实现
 * @details 通过函数指针调用适配层实现
 */

#include <stdlib.h>
#include <string.h>

#include "osal.h"
#include "osal_ops.h"

/* ==================== 内部辅助函数 ==================== */

static const osal_memory_ops_t* osal_get_memory_ops(void) {
    const osal_ops_t* ops = osal_get_ops();
    if (ops == NULL || ops->queue == NULL) {
        return NULL;
    }
    return ops->memory;
}

/* ==================== 内存操作接口实现 ==================== */

void* osal_memory_alloc(uint32_t size) {
    const osal_memory_ops_t* ops = osal_get_memory_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->alloc == NULL) {
        return NULL;
    }
    if (size == 0) {
        return NULL;
    }
#endif

    return ops->alloc(size);
}

osal_status_t osal_memory_free(void* ptr) {
    const osal_memory_ops_t* ops = osal_get_memory_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->free == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }
    if (ptr == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    return ops->free(ptr);
}

void* osal_memory_realloc(void* ptr, uint32_t size) {
    const osal_memory_ops_t* ops = osal_get_memory_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->realloc == NULL) {
        return NULL;
    }
#endif

    return ops->realloc(ptr, size);
}

void* osal_memory_alloc_aligned(uint32_t size, uint32_t alignment) {
    (void)alignment;
    /* 大多数RTOS不直接支持对齐分配，使用标准分配 */
    return osal_memory_alloc(size);
}

void* osal_memory_calloc(uint32_t num, uint32_t size) {
    const osal_memory_ops_t* ops = osal_get_memory_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->calloc == NULL) {
        return NULL;
    }
    if (num == 0 || size == 0) {
        return NULL;
    }
#endif

    return ops->calloc(num, size);
}

void* osal_memory_copy(void* dest, const void* src, uint32_t size) {
#if OSAL_CFG_PARAM_CHECK
    if (dest == NULL || src == NULL) {
        return NULL;
    }
#endif

    return memcpy(dest, src, size);
}

void* osal_memory_set(void* ptr, int value, uint32_t size) {
#if OSAL_CFG_PARAM_CHECK
    if (ptr == NULL) {
        return NULL;
    }
#endif

    return memset(ptr, value, size);
}

void* osal_memory_move(void* dest, const void* src, uint32_t size) {
#if OSAL_CFG_PARAM_CHECK
    if (dest == NULL || src == NULL) {
        return NULL;
    }
#endif

    return memmove(dest, src, size);
}

int osal_memory_compare(const void* ptr1, const void* ptr2, uint32_t size) {
#if OSAL_CFG_PARAM_CHECK
    if (ptr1 == NULL || ptr2 == NULL) {
        return -1;
    }
#endif

    return memcmp(ptr1, ptr2, size);
}

osal_status_t osal_memory_get_size(const void* ptr, uint32_t* size) {
    (void)ptr;
    (void)size;
    return OSAL_ERROR_NOT_SUPPORTED;
}

uint32_t osal_memory_get_free_size(void) {
    const osal_memory_ops_t* ops = osal_get_memory_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_free_size == NULL) {
        return 0;
    }
#endif

    return ops->get_free_size();
}

uint32_t osal_memory_get_minimum_free_size(void) {
    const osal_memory_ops_t* ops = osal_get_memory_ops();

#if OSAL_CFG_PARAM_CHECK
    if (ops == NULL || ops->get_minimum_free_size == NULL) {
        return 0;
    }
#endif

    return ops->get_minimum_free_size();
}

/* ==================== 内存池操作接口实现 ==================== */

osal_status_t osal_memory_pool_create(const osal_memory_pool_config_t* config,
                                      osal_memory_t* handle) {
    (void)config;
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_memory_pool_delete(osal_memory_t handle) {
    (void)handle;
    return OSAL_ERROR_NOT_SUPPORTED;
}

void* osal_memory_pool_alloc(osal_memory_t handle, osal_tick_t timeout) {
    (void)handle;
    (void)timeout;
    return NULL;
}

osal_status_t osal_memory_pool_free(osal_memory_t handle, void* ptr) {
    (void)handle;
    (void)ptr;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_memory_pool_get_available_blocks(osal_memory_t handle, uint32_t* count) {
    (void)handle;
    (void)count;
    return OSAL_ERROR_NOT_SUPPORTED;
}

osal_status_t osal_memory_pool_get_info(osal_memory_t handle, const char** name,
                                        uint32_t* total_size, uint32_t* block_size,
                                        uint32_t* free_blocks) {
    (void)handle;
    (void)name;
    (void)total_size;
    (void)block_size;
    (void)free_blocks;
    return OSAL_ERROR_NOT_SUPPORTED;
}

/* ==================== 内存统计信息 ==================== */

osal_status_t osal_memory_get_stats(osal_memory_stats_t* stats) {
#if OSAL_CFG_PARAM_CHECK
    if (stats == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    const osal_memory_ops_t* ops = osal_get_memory_ops();

    if (ops == NULL) {
        return OSAL_ERROR_NOT_INIT;
    }

    stats->total_size = 0;
    stats->used_size = 0;
    stats->free_size = (ops->get_free_size != NULL) ? ops->get_free_size() : 0;
    stats->min_free_size = (ops->get_minimum_free_size != NULL) ? ops->get_minimum_free_size() : 0;
    stats->alloc_count = 0;
    stats->free_count = 0;

    return OSAL_OK;
}

/* ==================== Hook函数实现 ==================== */

OSAL_WEAK void osal_memory_alloc_failed_hook(uint32_t size) {
    (void)size;
    /* 用户可重写 */
}

OSAL_WEAK void osal_memory_check_hook(void) { /* 用户可重写 */ }
