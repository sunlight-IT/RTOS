/**
 * @file example_mutex.c
 * @brief OSAL互斥锁使用示例
 * @details 演示如何使用互斥锁保护共享资源
 */

#include "osal.h"
#include <stdio.h>

/* ==================== 共享资源 ==================== */

/**
 * @brief 共享计数器
 */
static volatile int g_shared_counter = 0;

/**
 * @brief 互斥锁句柄
 */
static osal_mutex_t g_mutex;

/* ==================== 任务函数 ==================== */

/**
 * @brief 递增任务函数
 * @details 使用互斥锁保护共享计数器
 */
static void increment_task(void *param)
{
    int task_id = *(int *)param;

    while (1) {
        /* 获取互斥锁 */
        osal_status_t ret = osal_mutex_acquire(g_mutex, OSAL_WAIT_FOREVER);
        if (ret != OSAL_OK) {
            printf("Task %d: Mutex acquire failed: %s\n", task_id, osal_strerror(ret));
            continue;
        }

        /* 访问共享资源 */
        int old_value = g_shared_counter;
        g_shared_counter++;
        printf("Task %d: Counter %d -> %d\n", task_id, old_value, g_shared_counter);

        /* 释放互斥锁 */
        osal_mutex_release(g_mutex);

        /* 延时 */
        osal_task_delay(500);
    }
}

/* ==================== 主函数 ==================== */

int main(void)
{
    osal_status_t ret;
    osal_task_t task_handles[3];
    int task_ids[3] = {1, 2, 3};

    /* 初始化OSAL */
    ret = osal_init(OSAL_OS_FREERTOS);
    if (ret != OSAL_OK) {
        printf("OSAL init failed: %s\n", osal_strerror(ret));
        return -1;
    }

    printf("OSAL Mutex Example\n");
    printf("OSAL Version: %s\n\n", osal_get_version());

    /* 创建互斥锁 */
    osal_mutex_config_t mutex_config = {
        .name = "SharedCounterMutex",
        .inherit = 1  /* 启用优先级继承 */
    };

    ret = osal_mutex_create(&mutex_config, &g_mutex);
    if (ret != OSAL_OK) {
        printf("Mutex creation failed: %s\n", osal_strerror(ret));
        osal_deinit();
        return -1;
    }

    printf("Mutex created successfully\n");

    /* 创建多个任务同时访问共享资源 */
    for (int i = 0; i < 3; i++) {
        osal_task_config_t task_config = {
            .name = "IncrementTask",
            .func = increment_task,
            .param = &task_ids[i],
            .stack_size = 256,
            .priority = OSAL_PRIORITY_NORMAL,
            .time_slice = 0
        };

        ret = osal_task_create(&task_config, &task_handles[i]);
        if (ret != OSAL_OK) {
            printf("Task %d creation failed: %s\n", i + 1, osal_strerror(ret));
            osal_mutex_delete(g_mutex);
            osal_deinit();
            return -1;
        }

        printf("Task %d created successfully\n", i + 1);
    }

    printf("\nStarting tasks...\n\n");

    /* 启动调度器 */
    ret = osal_start();
    if (ret != OSAL_OK) {
        printf("OSAL start failed: %s\n", osal_strerror(ret));
    }

    return 0;
}
