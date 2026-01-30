/**
 * @file osal_port.c
 * @brief FreeRTOS适配层主文件
 * @details 包含初始化、反初始化和通用辅助函数
 */

#include "osal_port.h"

#include <stdlib.h>
#include <string.h>

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 检测当前是否在中断服务程序(ISR)中
 * @note 参考cmsis_os.c实现，使用__get_IPSR()获取中断状态
 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
/* ARM Cortex-M内核支持__get_IPSR() */
#include "cmsis_gcc.h"
int inHandlerMode(void) { return __get_IPSR() != 0; }
#else
/* 其他平台使用FreeRTOS提供的方式 */

int inHandlerMode(void) {
/* xPortIsInsideInterrupt是FreeRTOS内部函数，可能不可用 */
/* 使用替代方案：检查EXC_RETURN或使用port宏 */
#if configASSERT_DEFINED
    extern BaseType_t xPortIsInsideInterrupt(void);
    return xPortIsInsideInterrupt() != 0;
#else
    /* 默认返回0，表示不在中断中 */
    return 0;
#endif
}
#endif

/* Convert from CMSIS type osPriority to FreeRTOS priority number */
unsigned long osal_makeFreeRtosPriority(osal_priority_t priority) {
    unsigned long fpriority = OSAL_IDLE_PRIORITY;

    if (priority != osalPriorityError) {
        fpriority += (priority - osalPriorityIdle);
    }

    return fpriority;
}

/* ==================== 初始化和反初始化 ==================== */

osal_status_t osal_port_freertos_init(void) {
    /* FreeRTOS已经在vTaskStartScheduler之前初始化 */
    return OSAL_OK;
}

osal_status_t osal_port_freertos_deinit(void) {
    /* FreeRTOS由vTaskEndScheduler处理 */
    return OSAL_OK;
}
