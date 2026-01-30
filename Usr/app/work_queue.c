#include "work_queue.h"

#include "log/my_log.h"

#define LOOP_TABLE_MAX 10

static void work_queue_remove(work_queue_t* work_queue);

static void work_singal_process(work_node_t* work_node);
static void work_loop_process(work_node_t* work_node);
static void work_loop_check(work_node_t* work_node);
static void work_node_status_set(work_node_t* node, e_work_loop_status_t status);

static work_node_t* loop_table[LOOP_TABLE_MAX];
static uint8_t loop_table_index = 0;

work_node_data_t work_node[] = {{
    .mode = mode,
    .status = k_WORK_STATUS_RUNNING,
    .work_func = work_func,
    .arg = arg,
}

};

void work_queue_schedule(work_queue_t* work_queue) { osSignalSet(work_queue->work_thread, 0x02); }

static osal_mutex_t work_queue_lock;
static osal_queue_t work_queue_queue;
static osal_task_t work_queue_thread;
static osal_list_t work_queue_list;
/*
 * @brief 工作队列线程
 *
 * @param void* 工作队列参数指针，初始化时传入
 */
void work_queue_thread(void* pvParameters) {
    osal_task_notify_wait(0, 0x01, osal_SetBits,
                          OSAL_WAIT_FOREVER);  // 可以加入线程任务状态，可以挂起线程

    LOGI("work_queue_thread start");
    work_queue_t* work_queue = (work_queue_t*)pvParameters;
    work_node_t* work_node_now = NULL;
    work_node_t* work_node_last = NULL;
    uint8_t item_cnt = 0;
    while (1) {
        work_queue_remove(work_queue);

        if (pdFALSE == listLIST_IS_EMPTY(&work_queue->work_list)) {
            for (work_node_now = (work_node_t*)(listGET_HEAD_ENTRY(&work_queue->work_list));
                 item_cnt < listCURRENT_LIST_LENGTH(&work_queue->work_list); item_cnt++) {
                if (work_node_now != NULL) {
                    switch (work_node_now->mode) {
                        case k_WORK_NODE_LOOP:
                            work_loop_process(work_node_now);
                            break;
                        case k_WORK_NODE_SIGNAL:
                            work_singal_process(work_node_now);
                            break;
                    }
                }
                work_node_last = work_node_now;
                work_node_now = (work_node_t*)listGET_NEXT(&(work_node_now->list_item));
                work_loop_check(work_node_last);
            }
            item_cnt = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
 * @brief:  设置循环工作状态
 * @param:  loop_index 循环工作索引
 *          status 状态
 * @return: void
 */
static void work_loop_status_set(work_queue_t* work_queue, TickType_t loop_index,
                                 e_work_loop_status_t status) {
    uint8_t item_cnt = 0;

    xSemaphoreTake(work_queue->lock, portMAX_DELAY);
    for (work_node_t* work_node = (work_node_t*)(listGET_HEAD_ENTRY(&work_queue->work_list));
         item_cnt < listCURRENT_LIST_LENGTH(&work_queue->work_list); item_cnt++) {
        if (NULL == work_node) {
            continue;
        }
        if (loop_index == work_node->list_item.xItemValue) {
            work_node->status = status;
        }
    }
    xSemaphoreGive(work_queue->lock);
}

/*
 * @brief:  删除循环工作
 * @param:  loop_index 循环工作索引
 * @return: void
 */
void work_loop_task_del(work_queue_t* work_queue, TickType_t loop_index) {
    work_loop_status_set(work_queue, loop_index, k_WORK_STATUS_DELETED);
}

/*
 * @brief:  挂起循环工作
 * @param:  loop_index 循环工作索引
 * @return: void
 */
void work_loop_task_running(work_queue_t* work_queue, TickType_t loop_index) {
    work_loop_status_set(work_queue, loop_index, k_WORK_STATUS_RUNNING);
}

/*
 * @brief:  挂起循环工作
 * @param:  loop_index 循环工作索引
 * @return: void
 */
void work_loop_task_pending(work_queue_t* work_queue, TickType_t loop_index) {
    work_loop_status_set(work_queue, loop_index, k_WORK_STATUS_PEND);
}

/*
 * @brief 创建一个工作队列
 *
 * @param work_queue* 工作队列指针
 */

void work_queue_init(work_queue_t* work_queue) {
    work_queue->lock = xSemaphoreCreateMutex();

    osal_mutex_config_t mutex_config = {
        .name = "event_thread_mutex",
        .inherit = 1,
    };
    osal_mutex_create(&mutex_config, &work_queue_lock);

    osal_queue_config_t queue_config = {
        .name = "work_queue_queue",
        .max_items = 10,
        .item_size = sizeof(work_node_t*),
    };
    osal_queue_create(&queue_config, &work_queue_queue);

    osal_task_config_t task_config = {
        .name = "work_queue_thread",
        .func = (osal_task_func_t)work_queue_thread,
        .param = work_queue,
        .stack_size = 256 * sizeof(StackType_t),
        .priority = osalPriorityAboveNormal,
    };
    osal_task_create(&task_config, &work_queue_thread);
    // xTaskCreate(work_queue_thread, "work_queue_thread", 256, work_queue, 5,
    //             &(work_queue->work_thread));
    osal_list_config_t list_config = {
        .name = "work_queue_list",
    };
    osal_list_create(&list_config, &work_queue_list);

    // vListInitialise(&work_queue->work_list);
}

/*
 * @brief 添加一个工作
 *
 * @param work_queue*               工作队列指针
 * @param TickType_t xValue         工作任务id
 * @param void (*work_func)(void *) 工作执行函数
 * @param void *arg                 工作执行函数参数
 */
void work_queue_add(work_queue_t* work_queue, TickType_t xValue, void (*work_func)(void*),
                    e_work_node_mode_t mode, void* arg) {
    if (work_queue == NULL) {
        LOGE("work_queue_add work_queue is null");
        return;
    }

    if (OSAL_OK != osal_list_push_front(work_queue_list, &work_node)) {
        LOGE("work_queue_add malloc fail");
        return;
    }

    if (k_WORK_NODE_LOOP == mode) {
        work_node->status = k_WORK_STATUS_RUNNING;
    }

    xQueueSend(work_queue->queue, &work_node,
               portMAX_DELAY);  // 向工作队列中添加工作,使用也就是传递地址的方法进行数据传递
}

/*
 * @brief 取出一个工作
 *
 * @param work_queue* 工作队列指针
 */

// todo  链表结构体移植为osal链表
void work_queue_remove(work_queue_t* work_queue) {
    if (work_queue == NULL) {
        LOGE("work_queue_remove work_queue is null");
        return;
    }

    uint32_t work_node_addr;
    if (pdPASS == osal_queue_receive(work_queue_queue, &work_node_addr, 100))  // 接收工作节点地址
    {
        osal_mutex_acquire(work_queue_lock, portMAX_DELAY);
        vListInsert(&work_queue_list, &(((work_node_t*)work_node_addr)->list_item));  // todo
        osal_mutex_release(work_queue_lock);
    }  // 添加循环工作运行一次工作，就不可以使用永久阻塞了
}
// todo

/*
 * @brief 运行一次工作的处理
 * @description 处理完成就释放空间
 * @param work_node_t* 工作节点指针
 * @return void
 */
void work_singal_process(work_node_t* work_node) {
    work_node->work_func(work_node->arg);
    work_node_status_set(work_node, k_WORK_STATUS_DELETED);
}

/*
 * @brief：       运行循环工作处理
 * @description： 运行loop表中的工作节点
 * @param：       work_node_t* 工作节点指针
 * @return：      void
 */
void work_loop_process(work_node_t* node) {
    if (k_WORK_STATUS_RUNNING == node->status &&  //
        node->work_func != NULL) {
        node->work_func(node->arg);
    }
}

/*
 * @brief：       检测循环工作状态
 * @description： 将状态为删除的工作节点从表中移除
 * @param：       void
 * @return：      void
 */
void work_loop_check(work_node_t* node) {
    if (NULL == node) {
        return;
    }

    if (k_WORK_STATUS_DELETED == node->status) {
        uxListRemove(&node->list_item);
        vPortFree(node);  // 释放工作节点
    }
}

void work_node_status_set(work_node_t* node, e_work_loop_status_t status) { node->status = status; }