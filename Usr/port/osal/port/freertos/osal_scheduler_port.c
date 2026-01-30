/**
 * @file osal_scheduler_port.c
 * @brief FreeRTOS璋冨害鍣ㄦ帶鍒剁粍浠堕�傞厤瀹炵幇
 * @details 瀹炵幇FreeRTOS鍒癘SAL鐨勮皟搴﹀櫒鎺у埗閫傞厤鍔熻兘
 */

#include "osal_scheduler_port.h"

/* ==================== 璋冨害鍣ㄦ帶鍒堕�傞厤瀹炵幇 ==================== */

/**
 * @file osal_scheduler_port.c
 * @brief FreeRTOS调度器控制组件适配实现
 * @details 实现FreeRTOS到OSAL的调度器控制适配功能
 */

#include "osal_scheduler_port.h"

/* ==================== 调度器控制适配实现 ==================== */

osal_status_t osal_port_freertos_scheduler_suspend(void) {
    vTaskSuspendAll();
    return OSAL_OK;
}

osal_status_t osal_port_freertos_scheduler_resume(void) {
    if (xTaskResumeAll() == pdTRUE) {
        taskYIELD();
    }
    return OSAL_OK;
}
