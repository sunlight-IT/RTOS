/**
 * @file example_basic.c
 * @brief OSAL基本使用示例
 * @details 演示如何使用OSAL进行任务、队列、互斥锁等基本操作
 */

#include "osal.h"
#include <stdio.h>

/* ==================== 任务配置 ==================== */

/**
 * @brief 生产者任务参数
 */
typedef struct {
    osal_queue_t queue;
    int value;
} producer_task_param_t;

/**
 * @brief 消费者任务参数
 */
typedef struct {
    osal_queue_t queue;
} consumer_task_param_t;

/* ==================== 任务函数 ==================== */

/**
 * @brief 生产者任务函数
 */
static void producer_task(void *param)
{
    producer_task_param_t *task_param = (producer_task_param_t *)param;

    while (1) {
        /* 生产数据 */
        task_param->value++;
        printf("Producer: Sending value %d\n", task_param->value);

        /* 发送到队列 */
        osal_status_t ret = osal_queue_send(task_param->queue, &task_param->value, OSAL_WAIT_FOREVER);
        if (ret != OSAL_OK) {
            printf("Producer: Send failed, error: %s\n", osal_strerror(ret));
        }

        /* 延时1秒 */
        osal_task_delay(1000);
    }
}

/**
 * @brief 消费者任务函数
 */
static void consumer_task(void *param)
{
    consumer_task_param_t *task_param = (consumer_task_param_t *)param;
    int received_value;

    while (1) {
        /* 从队列接收 */
        osal_status_t ret = osal_queue_receive(task_param->queue, &received_value, OSAL_WAIT_FOREVER);
        if (ret == OSAL_OK) {
            printf("Consumer: Received value %d\n", received_value);
        } else {
            printf("Consumer: Receive failed, error: %s\n", osal_strerror(ret));
        }
    }
}

/**
 * @brief 统计任务函数
 */
static void stats_task(void *param)
{
    (void)param;

    while (1) {
        /* 每5秒打印一次统计信息 */
        osal_task_delay(5000);

        osal_info_t info;
        if (osal_get_info(&info) == OSAL_OK) {
            printf("\n=== OSAL Statistics ===\n");
            printf("Version: %s\n", info.version);
            printf("State: %d\n", info.state);
            printf("Tasks: %u\n", info.task_count);
            printf("Queues: %u\n", info.queue_count);
            printf("Mutexes: %u\n", info.mutex_count);
            printf("Semaphores: %u\n", info.semaphore_count);
            printf("Events: %u\n", info.event_count);
            printf("Free Memory: %u bytes\n", osal_memory_get_free_size());
            printf("========================\n\n");
        }
    }
}

/* ==================== 主函数 ==================== */

/**
 * @brief 主函数
 */
int main(void)
{
    osal_status_t ret;
    osal_task_t producer_handle, consumer_handle, stats_handle;
    osal_queue_t queue;

    /* 初始化OSAL */
    ret = osal_init(OSAL_OS_FREERTOS);
    if (ret != OSAL_OK) {
        printf("OSAL init failed: %s\n", osal_strerror(ret));
        return -1;
    }

    printf("OSAL initialized successfully\n");
    printf("OSAL Version: %s\n", osal_get_version());

    /* 创建队列 */
    osal_queue_config_t queue_config = {
        .name = "DataQueue",
        .max_items = 10,
        .item_size = sizeof(int)
    };

    ret = osal_queue_create(&queue_config, &queue);
    if (ret != OSAL_OK) {
        printf("Queue creation failed: %s\n", osal_strerror(ret));
        osal_deinit();
        return -1;
    }

    printf("Queue created successfully\n");

    /* 创建任务参数 */
    static producer_task_param_t producer_param;
    static consumer_task_param_t consumer_param;

    producer_param.queue = queue;
    producer_param.value = 0;
    consumer_param.queue = queue;

    /* 创建生产者任务 */
    osal_task_config_t producer_config = {
        .name = "Producer",
        .func = producer_task,
        .param = &producer_param,
        .stack_size = 512,
        .priority = OSAL_PRIORITY_NORMAL,
        .time_slice = 0
    };

    ret = osal_task_create(&producer_config, &producer_handle);
    if (ret != OSAL_OK) {
        printf("Producer task creation failed: %s\n", osal_strerror(ret));
        osal_queue_delete(queue);
        osal_deinit();
        return -1;
    }

    printf("Producer task created successfully\n");

    /* 创建消费者任务 */
    osal_task_config_t consumer_config = {
        .name = "Consumer",
        .func = consumer_task,
        .param = &consumer_param,
        .stack_size = 512,
        .priority = OSAL_PRIORITY_NORMAL,
        .time_slice = 0
    };

    ret = osal_task_create(&consumer_config, &consumer_handle);
    if (ret != OSAL_OK) {
        printf("Consumer task creation failed: %s\n", osal_strerror(ret));
        osal_task_delete(producer_handle);
        osal_queue_delete(queue);
        osal_deinit();
        return -1;
    }

    printf("Consumer task created successfully\n");

    /* 创建统计任务 */
    osal_task_config_t stats_config = {
        .name = "Statistics",
        .func = stats_task,
        .param = NULL,
        .stack_size = 256,
        .priority = OSAL_PRIORITY_LOW,
        .time_slice = 0
    };

    ret = osal_task_create(&stats_config, &stats_handle);
    if (ret != OSAL_OK) {
        printf("Statistics task creation failed: %s\n", osal_strerror(ret));
        osal_task_delete(producer_handle);
        osal_task_delete(consumer_handle);
        osal_queue_delete(queue);
        osal_deinit();
        return -1;
    }

    printf("Statistics task created successfully\n");
    printf("Starting OSAL scheduler...\n\n");

    /* 启动调度器 */
    ret = osal_start();
    if (ret != OSAL_OK) {
        printf("OSAL start failed: %s\n", osal_strerror(ret));
    }

    /* 永不返回 */
    return 0;
}

/* ==================== Hook函数实现 ==================== */

/**
 * @brief OSAL初始化完成Hook
 */
void osal_init_hook(void)
{
    printf("osal_init_hook: Custom initialization\n");
}

/**
 * @brief OSAL启动前Hook
 */
void osal_start_hook(void)
{
    printf("osal_start_hook: About to start scheduler\n");
}

/**
 * @brief 内存分配失败Hook
 */
void osal_malloc_failed_hook(uint32_t size)
{
    printf("osal_malloc_failed_hook: Failed to allocate %u bytes\n", (unsigned int)size);
}
