/**
 * @file example_event.c
 * @brief OSAL事件使用示例
 * @details 演示如何使用事件标志进行任务间通信
 */

#include "osal.h"
#include <stdio.h>

/* ==================== 事件标志定义 ==================== */

#define EVENT_FLAG_DATA_READY    (1 << 0)  /**< 数据就绪 */
#define EVENT_FLAG_ERROR         (1 << 1)  /**< 错误发生 */
#define EVENT_FLAG_COMPLETE      (1 << 2)  /**< 操作完成 */
#define EVENT_FLAG_SHUTDOWN      (1 << 3)  /**< 关闭请求 */

/* ==================== 事件句柄 ==================== */

static osal_event_t g_event;

/* ==================== 任务函数 ==================== */

/**
 * @brief 事件等待任务
 * @details 等待多个事件标志，根据不同标志执行不同操作
 */
static void event_waiter_task(void *param)
{
    int task_id = *(int *)param;
    osal_event_flags_t events;

    printf("Task %d: Starting event waiter\n", task_id);

    while (1) {
        /* 等待任意事件（超时5秒） */
        osal_status_t ret = osal_event_wait(
            g_event,
            EVENT_FLAG_DATA_READY | EVENT_FLAG_ERROR | EVENT_FLAG_COMPLETE | EVENT_FLAG_SHUTDOWN,
            OSAL_EVENT_WAIT_ANY,
            &events,
            5000
        );

        if (ret == OSAL_OK) {
            if (events & EVENT_FLAG_DATA_READY) {
                printf("Task %d: Data ready event received\n", task_id);
            }

            if (events & EVENT_FLAG_ERROR) {
                printf("Task %d: Error event received\n", task_id);
            }

            if (events & EVENT_FLAG_COMPLETE) {
                printf("Task %d: Complete event received\n", task_id);
            }

            if (events & EVENT_FLAG_SHUTDOWN) {
                printf("Task %d: Shutdown event received, exiting\n", task_id);
                break;
            }
        } else if (ret == OSAL_ERROR_TIMEOUT) {
            printf("Task %d: Event wait timeout\n", task_id);
        } else {
            printf("Task %d: Event wait failed: %s\n", task_id, osal_strerror(ret));
        }
    }

    /* 删除自己 */
    osal_task_delete(NULL);
}

/**
 * @brief 事件发送任务
 * @details 定时发送不同的事件标志
 */
static void event_sender_task(void *param)
{
    int counter = 0;

    printf("Sender: Starting event sender\n");

    while (1) {
        counter++;

        /* 发送不同的事件 */
        switch (counter % 4) {
            case 1:
                printf("Sender: Setting DATA_READY event\n");
                osal_event_set(g_event, EVENT_FLAG_DATA_READY);
                break;

            case 2:
                printf("Sender: Setting COMPLETE event\n");
                osal_event_set(g_event, EVENT_FLAG_COMPLETE);
                break;

            case 3:
                printf("Sender: Setting ERROR event\n");
                osal_event_set(g_event, EVENT_FLAG_ERROR);
                /* 清除错误 */
                osal_task_delay(500);
                printf("Sender: Clearing ERROR event\n");
                osal_event_clear(g_event, EVENT_FLAG_ERROR);
                break;

            case 0:
                printf("Sender: Setting multiple events\n");
                osal_event_set(g_event, EVENT_FLAG_DATA_READY | EVENT_FLAG_COMPLETE);
                break;
        }

        /* 延时 */
        osal_task_delay(2000);

        /* 发送10次后发送关闭事件 */
        if (counter >= 10) {
            printf("Sender: Setting SHUTDOWN event\n");
            osal_event_set(g_event, EVENT_FLAG_SHUTDOWN);
            break;
        }
    }

    /* 删除自己 */
    osal_task_delete(NULL);
}

/* ==================== 主函数 ==================== */

int main(void)
{
    osal_status_t ret;
    osal_task_t waiter_handles[3];
    osal_task_t sender_handle;
    int task_ids[3] = {1, 2, 3};

    /* 初始化OSAL */
    ret = osal_init(OSAL_OS_FREERTOS);
    if (ret != OSAL_OK) {
        printf("OSAL init failed: %s\n", osal_strerror(ret));
        return -1;
    }

    printf("OSAL Event Example\n");
    printf("OSAL Version: %s\n\n", osal_get_version());

    /* 创建事件组 */
    osal_event_config_t event_config = {
        .name = "ControlEvents"
    };

    ret = osal_event_create(&event_config, &g_event);
    if (ret != OSAL_OK) {
        printf("Event creation failed: %s\n", osal_strerror(ret));
        osal_deinit();
        return -1;
    }

    printf("Event group created successfully\n");

    /* 创建3个等待任务 */
    for (int i = 0; i < 3; i++) {
        osal_task_config_t task_config = {
            .name = "EventWaiter",
            .func = event_waiter_task,
            .param = &task_ids[i],
            .stack_size = 256,
            .priority = OSAL_PRIORITY_NORMAL,
            .time_slice = 0
        };

        ret = osal_task_create(&task_config, &waiter_handles[i]);
        if (ret != OSAL_OK) {
            printf("Waiter task %d creation failed: %s\n", i + 1, osal_strerror(ret));
            osal_event_delete(g_event);
            osal_deinit();
            return -1;
        }

        printf("Waiter task %d created successfully\n", i + 1);
    }

    /* 创建发送任务 */
    osal_task_config_t sender_config = {
        .name = "EventSender",
        .func = event_sender_task,
        .param = NULL,
        .stack_size = 256,
        .priority = OSAL_PRIORITY_NORMAL,
        .time_slice = 0
    };

    ret = osal_task_create(&sender_config, &sender_handle);
    if (ret != OSAL_OK) {
        printf("Sender task creation failed: %s\n", osal_strerror(ret));
        osal_event_delete(g_event);
        osal_deinit();
        return -1;
    }

    printf("Sender task created successfully\n");
    printf("\nStarting scheduler...\n\n");

    /* 启动调度器 */
    ret = osal_start();
    if (ret != OSAL_OK) {
        printf("OSAL start failed: %s\n", osal_strerror(ret));
    }

    return 0;
}
