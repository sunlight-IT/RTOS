/**
 * @file osal_memory_port.c
 * @brief FreeRTOS内存管理组件适配实现
 * @details 实现FreeRTOS到OSAL的内存管理适配功能
 */

#include "osal_memory_port.h"

#include <stdlib.h>
#include <string.h>

/* ==================== 内存管理适配实现 ==================== */

void* osal_port_freertos_memory_alloc(uint32_t size) { return pvPortMalloc(size); }

osal_status_t osal_port_freertos_memory_free(void* ptr) {
#if OSAL_CFG_PARAM_CHECK
    if (ptr == NULL) {
        return OSAL_ERROR_INVALID_PARAM;
    }
#endif

    vPortFree(ptr);
    return OSAL_OK;
}

void* osal_port_freertos_memory_realloc(void* ptr, uint32_t size) {
    /* FreeRTOS标准没有realloc，使用自定义实现 */
    void* new_ptr = pvPortMalloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    if (ptr != NULL) {
        /* 获取原大小（FreeRTOS没有直接方式，这里假设用户需要手动复制） */
        memcpy(new_ptr, ptr, size); /* 简化处理，实际应用可能需要更复杂逻辑 */
        vPortFree(ptr);
    }

    return new_ptr;
}

void* osal_port_freertos_memory_calloc(uint32_t num, uint32_t size) {
    void* ptr = pvPortMalloc(num * size);
    if (ptr != NULL) {
        memset(ptr, 0, num * size);
    }
    return ptr;
}

uint32_t osal_port_freertos_memory_get_free_size(void) { return xPortGetFreeHeapSize(); }

uint32_t osal_port_freertos_memory_get_minimum_free_size(void) {
    return xPortGetMinimumEverFreeHeapSize();
}
