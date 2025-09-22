#pragma once
#include "cmsis_os.h"

/*
 * Work Queue
 * 需要的数据和方法有
 * 1. 队列
 * 2. 工作线程
 * 2. 工作函数链表
 * 3. 保护锁/高频任务使用信号量
 * 4. 或者高频任务不使用工作队列
 *
 * 方法
 * 1. 初始化
 *      - 创建队列
 *      - 创建信号量
 *      - 创建工作链表
 *
 * 2. 添加工作
 *       - 创建工作节点
 *       - 添加工作节点到工作链表
 * 3. 运行工作(工作处理器)
 *       - 从队列中取出一个工作节点
 *       - 运行工作节点
 * 4. 删除工作
 *       - 从工作链表中删除工作节点
 *
 */

typedef struct _work_node_t {
    ListItem_t list_item;
    void (*work_func)(void *);
    void *arg;
} work_node_t;

typedef struct _work_queue_t {
    List_t work_list;
    QueueHandle_t lock;
    QueueHandle_t work_queue;
    TaskHandle_t work_thread;
} work_queue_t;

void work_queue_init(work_queue_t *work_queue);
void work_queue_add(work_queue_t *work_queue, TickType_t xValue, void (*work_func)(void *),
                    void *arg);
