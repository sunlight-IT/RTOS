/**
 * @file example_semaphore.c
 * @brief OSAL信号量使用示例
 * @details 演示如何使用信号量进行任务同步和资源管理
 */

#include "osal.h"
#include <stdio.h>

/* ==================== 信号量句柄 ==================== */

static osal_semaphore_t g_binary_sem;
static osal_semaphore_t g_counting_sem;

/* ==================== 任务函数 ==================== */

/**
 * @brief 生产者任务 - 使用二值信号量同步
 */
static void producer_task(void *param)
{
    int counter = 0;

    while (1) {
        /* 生产数据 */
        counter++;
        printf("Producer: Produced item %d\n", counter);

        /* 释放信号量，通知消费者 */
        osal_status_t ret = osal_semaphore_release(g_binary_sem);
        if (ret != OSAL_OK) {
            printf("Producer: Semaphore release failed: %s\n", osal_strerror(ret));
        }

        /* 延时 */
        osal_task_delay(1000);
    }
}

/**
 * @brief 消费者任务 - 使用二值信号量同步
 */
static void consumer_task(void *param)
{
    while (1) {
        /* 等待信号量 */
        osal_status_t ret = osal_semaphore_acquire(g_binary_sem, OSAL_WAIT_FOREVER);
        if (ret == OSAL_OK) {
            printf("Consumer: Processing item\n");
        } else {
            printf("Consumer: Semaphore acquire failed: %s\n", osal_strerror(ret));
        }
    }
}

/**
 * @brief 资源访问任务 - 使用计数信号量管理资源
 */
static void resource_task(void *param)
{
    int task_id = *(int *)param;

    while (1) {
        /* 请求资源（最多允许3个任务同时访问） */
        printf("Task %d: Waiting for resource...\n", task_id);

        osal_status_t ret = osal_semaphore_acquire(g_counting_sem, OSAL_WAIT_FOREVER);
        if (ret == OSAL_OK) {
            printf("Task %d: Resource acquired, working...\n", task_id);

            /* 使用资源 */
            osal_task_delay(2000);

            printf("Task %d: Releasing resource\n", task_id);

            /* 释放资源 */
            osal_semaphore_release(g_counting_sem);
        } else {
            printf("Task %d: Resource acquire failed: %s\n", task_id, osal_strerror(ret));
        }

        /* 延时后再次请求 */
        osal_task_delay(1000);
    }
}

/* ==================== 主函数 ==================== */

int main(void)
{
    osal_status_t ret;
    osal_task_t producer_handle, consumer_handle;
    osal_task_t resource_handles[5];
    int task_ids[5] = {1, 2, 3, 4, 5};

    /* 初始化OSAL */
    ret = osal_init(OSAL_OS_FREERTOS);
    if (ret != OSAL_OK) {
        printf("OSAL init failed: %s\n", osal_strerror(ret));
        return -1;
    }

    printf("OSAL Semaphore Example\n");
    printf("OSAL Version: %s\n\n", osal_get_version());

    /* 创建二值信号量（用于同步） */
    osal_semaphore_config_t binary_sem_config = {
        .name = "BinarySemaphore",
        .type = OSAL_SEMAPHORE_BINARY,
        .max_count = 1,
        .init_count = 0
    };

    ret = osal_semaphore_create(&binary_sem_config, &g_binary_sem);
    if (ret != OSAL_OK) {
        printf("Binary semaphore creation failed: %s\n", osal_strerror(ret));
        osal_deinit();
        return -1;
    }

    printf("Binary semaphore created successfully\n");

    /* 创建计数信号量（用于资源管理，最多3个资源） */
    osal_semaphore_config_t counting_sem_config = {
        .name = "CountingSemaphore",
        .type = OSAL_SEMAPHORE_COUNTING,
        .max_count = 3,
        .init_count = 3
    };

    ret = osal_semaphore_create(&counting_sem_config, &g_counting_sem);
    if (ret != OSAL_OK) {
        printf("Counting semaphore creation failed: %s\n", osal_strerror(ret));
        osal_semaphore_delete(g_binary_sem);
        osal_deinit();
        return -1;
    }

    printf("Counting semaphore created successfully\n");

    /* 创建生产者任务 */
    osal_task_config_t producer_config = {
        .name = "Producer",
        .func = producer_task,
        .param = NULL,
        .stack_size = 256,
        .priority = OSAL_PRIORITY_NORMAL,
        .time_slice = 0
    };

    ret = osal_task_create(&producer_config, &producer_handle);
    if (ret != OSAL_OK) {
        printf("Producer task creation failed: %s\n", osal_strerror(ret));
        osal_semaphore_delete(g_binary_sem);
        osal_semaphore_delete(g_counting_sem);
        osal_deinit();
        return -1;
    }

    printf("Producer task created successfully\n");

    /* 创建消费者任务 */
    osal_task_config_t consumer_config = {
        .name = "Consumer",
        .func = consumer_task,
        .param = NULL,
        .stack_size = 256,
        .priority = OSAL_PRIORITY_NORMAL,
        .time_slice = 0
    };

    ret = osal_task_create(&consumer_config, &consumer_handle);
    if (ret != OSAL_OK) {
        printf("Consumer task creation failed: %s\n", osal_strerror(ret));
        osal_task_delete(producer_handle);
        osal_semaphore_delete(g_binary_sem);
        osal_semaphore_delete(g_counting_sem);
        osal_deinit();
        return -1;
    }

    printf("Consumer task created successfully\n");

    /* 创建5个资源访问任务 */
    for (int i = 0; i < 5; i++) {
        osal_task_config_t task_config = {
            .name = "ResourceTask",
            .func = resource_task,
            .param = &task_ids[i],
            .stack_size = 256,
            .priority = OSAL_PRIORITY_NORMAL,
            .time_slice = 0
        };

        ret = osal_task_create(&task_config, &resource_handles[i]);
        if (ret != OSAL_OK) {
            printf("Resource task %d creation failed: %s\n", i + 1, osal_strerror(ret));
            /* 清理 */
            osal_semaphore_delete(g_binary_sem);
            osal_semaphore_delete(g_counting_sem);
            osal_deinit();
            return -1;
        }

        printf("Resource task %d created successfully\n", i + 1);
    }

    printf("\nStarting scheduler...\n\n");

    /* 启动调度器 */
    ret = osal_start();
    if (ret != OSAL_OK) {
        printf("OSAL start failed: %s\n", osal_strerror(ret));
    }

    return 0;
}
