#pragma once

#include "cmsis_os.h"

/*
* work_timer.h
*
* 创建者：刘日毅
* 创建时间：2021.09.05
* 数据与方法
* 数据
* 1. 定时器句柄 TimerHandle_t work_timer;
* 2. 定时器函数链表 work_list;
* 方法
* 1. 创建定时器 work_timer_init(void)
* 2. 添加定时器 work_timer_add(TickType_t xValue, void (*work_func)(void *), void *arg)
* 3. 删除定时器 work_timer_del(void (*work_func)(void *))
* 4. 启动定时器 work_timer_start(void)
* 5. 停止定时器 work_timer_stop(void)

*/

typedef struct _work_timer_node_t {
    ListItem_t list_item;
    void (*work_func)(void *);
    void *arg;
    TickType_t xPassTime;
    uint32_t id;
} work_timer_node_t;

typedef struct _work_timer_t {
    TimerHandle_t work_timer;
    List_t work_time_list;
    TickType_t xTickCount;
    QueueHandle_t lock;
} work_timer_t;

void work_timer_init(work_timer_t *work_timer);
void work_timer_node_add(work_timer_t *work_timer, uint32_t id, TickType_t xItemValue,
                         void (*work_func)(void *), void *arg);
void work_timer_node_remove(work_timer_t *work_timer, uint32_t id);
void work_timer_del(work_timer_t *work_timer);
void work_timer_remove_all(work_timer_t *work_timer);
void work_timer_start(work_timer_t *work_timer, TickType_t xProcessTicks);
