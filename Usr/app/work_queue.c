#include "work_queue.h"

#include "log/my_log.h"

static void work_queue_remove(work_queue_t *work_queue);

/*
 * @brief 工作队列线程
 *
 * @param void* 工作队列参数指针，初始化时传入
 */
void work_queue_thread(void *pvParameters) {
    work_queue_t *work_queue = (work_queue_t *)pvParameters;

    while (1) {
        work_queue_remove(work_queue);
        if (listLIST_IS_EMPTY(&work_queue->work_list)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            work_node_t *work_node = (work_node_t *)(listGET_HEAD_ENTRY(&work_queue->work_list));
            if (work_node != NULL) {
                work_node->work_func(work_node->arg);
                uxListRemove(&work_node->list_item);
                vPortFree(work_node);  // 释放工作节点
            }
        }
    }
}

/*
 * @brief 创建一个工作队列
 *
 * @param work_queue* 工作队列指针
 */
void work_queue_init(work_queue_t *work_queue) {
    work_queue->lock = xSemaphoreCreateMutex();
    // vQueueAddToRegistry(work_queue->sem, "work_queue_sem");
    work_queue->work_queue = xQueueCreate(10, sizeof(work_node_t *));
    // vQueueAddToRegistry(work_queue->work_queue, "work_queue");
    xTaskCreate(work_queue_thread, "work_queue_thread", 1024, work_queue, 5,
                &work_queue->work_thread);

    vListInitialise(&work_queue->work_list);
}

/*
 * @brief 添加一个工作
 *
 * @param work_queue*               工作队列指针
 * @param TickType_t xValue         列表项值延时(优先级)
 * @param void (*work_func)(void *) 工作执行函数
 * @param void *arg                 工作执行函数参数
 */
void work_queue_add(work_queue_t *work_queue, TickType_t xValue, void (*work_func)(void *),
                    void *arg) {
    if (work_queue == NULL) {
        LOGE("work_queue_add work_queue is null");
        return;
    }

    work_node_t *work_node = pvPortMalloc(sizeof(work_node_t));  // 动态开辟列表项

    if (work_node == NULL) {
        LOGE("work_queue_add malloc fail");
        return;
    }

    work_node->work_func = work_func;
    work_node->arg = arg;
    listSET_LIST_ITEM_VALUE(&work_node->list_item, xValue);

    xQueueSend(work_queue->work_queue, &work_node,
               portMAX_DELAY);  // 向工作队列中添加工作,使用0拷贝也就是传递地址的方法进行数据传递
}

/*
 * @brief 删除一个工作
 *
 * @param work_queue* 工作队列指针
 */
void work_queue_remove(work_queue_t *work_queue) {
    if (work_queue == NULL) {
        LOGE("work_queue_remove work_queue is null");
        return;
    }

    uint32_t work_node_addr;
    xQueueReceive(work_queue->work_queue, &work_node_addr, portMAX_DELAY);  // 接收工作节点地址

    xSemaphoreTake(work_queue->lock, portMAX_DELAY);
    vListInsert(&work_queue->work_list, &(((work_node_t *)work_node_addr)->list_item));
    xSemaphoreGive(work_queue->lock);
}
