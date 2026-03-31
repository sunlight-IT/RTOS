#include "work_queue.h"

#include "log/my_log.h"

#include <stdbool.h>

#define LOOP_TABLE_MAX 10

static void work_queue_remove(void);

static void work_singal_process(work_node_t *work_node);
static void work_loop_process(work_node_t *work_node);
static void work_loop_check(work_node_t *work_node);
static void work_node_status_set(work_node_t *node, e_work_loop_status_t status);

static work_node_t *loop_table[LOOP_TABLE_MAX];
static uint8_t loop_table_index = 0;

static work_queue_t s_work_queue;
static zThreadOS_t s_thread_os;

void work_queue_schedule(void) { zThread_schedule(s_work_queue.work_thread); }
/*
 * @brief 工作队列线程
 *
 * @param void* 工作队列参数指针，初始化时传入
 */


void work_queue_thread(void *pvParameters) {
    LOGI("work_queue_thread start");
    // work_queue_t *work_queue = (work_queue_t *)pvParameters;
 static work_node_t *work_node_now ;
 static work_node_t *work_node_last ;
 static uint8_t item_cnt; 

        work_queue_remove();
        if (pdFALSE == listLIST_IS_EMPTY(&s_work_queue.work_list)) {
            for (work_node_now = (work_node_t *)(listGET_HEAD_ENTRY(&s_work_queue.work_list));
                 item_cnt < listCURRENT_LIST_LENGTH(&s_work_queue.work_list); item_cnt++) {
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
                work_node_now = (work_node_t *)listGET_NEXT(&(work_node_now->list_item));
                work_loop_check(work_node_last);
            }
            work_node_now = NULL;
            work_node_last = NULL;
            item_cnt = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    
}

/*
 * @brief:  设置循环工作状态
 * @param:  loop_index 循环工作索引
 *          status 状态
 * @return: void
 */
static void work_loop_status_set( TickType_t loop_index,
                                 e_work_loop_status_t status) {
    uint8_t item_cnt = 0;

    if (s_thread_os.queue == NULL) {
        LOGE("work_loop_status_set work_queue is null");
        return;
    }

    osMutexAcquire(s_work_queue.lock, portMAX_DELAY);
    for (work_node_t *work_node = (work_node_t *)(listGET_HEAD_ENTRY(&s_work_queue.work_list));
         item_cnt < listCURRENT_LIST_LENGTH(&s_work_queue.work_list); item_cnt++) {
        if (NULL == work_node) {
            continue;
        }
        if (loop_index == work_node->list_item.xItemValue) {
            work_node->status = status;
        }
    }
    osMutexRelease(s_work_queue.lock);
}

/*
 * @brief:  删除循环工作
 * @param:  loop_index 循环工作索引
 * @return: void
 */
void work_loop_task_del( TickType_t loop_index) {
    work_loop_status_set( loop_index, k_WORK_STATUS_DELETED);
}

/*
 * @brief:  挂起循环工作
 * @param:  loop_index 循环工作索引
 * @return: void
 */
void work_loop_task_running( TickType_t loop_index) {
    work_loop_status_set( loop_index, k_WORK_STATUS_RUNNING);
}

/*
 * @brief:  挂起循环工作
 * @param:  loop_index 循环工作索引
 * @return: void
 */
void work_loop_task_pending( TickType_t loop_index) {
    work_loop_status_set( loop_index, k_WORK_STATUS_PEND);
}

/*
 * @brief 创建一个工作队列
 *
 * @param work_queue* 工作队列指针
 */
void work_queue_init(void) {

    uint32_t err = false;

    // osMutexAttr_t mutex_attr = {
    //     .name = "work_queue_mutex",
    //     .attr_bits = osMutexPrioInherit,
    //     .cb_mem = NULL,
    //     .cb_size = 0,
    // };
    // s_work_queue.lock = osMutexNew(&mutex_attr);
    // if (s_work_queue.lock == NULL) {
    //     return err;
    // }
    // // vQueueAddToRegistry(work_queue->sem, "work_queue_sem");
    // osMessageQueueAttr_t queue_attr = {
    //     .name = "work_queue_queue",
    //     .attr_bits = 0,
    //     .cb_mem = NULL,//用于静态队列内存使用
    //     .cb_size = 0,
    //     .mq_mem = NULL,//用于静态队列内存使用
    //     .mq_size = 0,
    // };
    // s_work_queue.queue = osMessageQueueNew(10, sizeof(work_node_t *), &queue_attr);
    // // vQueueAddToRegistry(work_queue->work_queue, "work_queue");
    // if(s_work_queue.queue == NULL) {
    //     return err;
    // }

    err = zThread_create(&s_thread_os, "work_queue_thread", work_queue_thread,
                             osPriorityNormal, sizeof(work_node_t *));

    if (err) {
        LOGE("work_queue_thread zThread_create error");
    }

    vListInitialise(&s_work_queue.work_list);
}

/*
 * @brief 添加一个工作
 *
 * @param work_queue*               工作队列指针
 * @param TickType_t xValue         工作任务id
 * @param void (*work_func)(void *) 工作执行函数
 * @param void *arg                 工作执行函数参数
 */
void work_queue_add( TickType_t xValue, void (*work_func)(void *),
                    e_work_node_mode_t mode, void *arg) {
    if (s_thread_os.queue == NULL) {
        LOGE("work_queue_add queue is null");
        return;
    }

    //todo 后期要换成内存池接口而不能耦合FreeRTOS
    work_node_t *work_node = pvPortMalloc(sizeof(work_node_t));  // 动态开辟列表项
    //todo 后期要换成内存池接口而不能耦合FreeRTOS

    if (work_node == NULL) {
        LOGE("work_queue_add malloc fail");
        return;
    }

    work_node->work_func = work_func;
    work_node->arg = arg;
    work_node->mode = mode;

    if (k_WORK_NODE_LOOP == mode) {
        work_node->status = k_WORK_STATUS_RUNNING;
    }
    listSET_LIST_ITEM_VALUE(&work_node->list_item, xValue);

    osMessageQueuePut(s_thread_os.queue, &work_node, 0,
               portMAX_DELAY);  // 向工作队列中添加工作,使用也就是传递地址的方法进行数据传递
}

/*
 * @brief 取出一个工作
 *
 * @param work_queue* 工作队列指针
 */
void work_queue_remove(void) {
    if (s_thread_os.queue == NULL) {
        LOGE("work_queue_remove work_queue is null");
        return;
    }

    uint32_t work_node_addr;
    if (pdPASS == osMessageQueueGet(s_thread_os.queue, &work_node_addr, 0, 100))  // 接收工作节点地址
    {
        osMutexAcquire(s_thread_os.mutex, portMAX_DELAY);
        vListInsert(&s_work_queue.work_list, &(((work_node_t *)work_node_addr)->list_item));
        osMutexRelease(s_thread_os.mutex);
    }  // 添加循环工作运行一次工作，就不可以使用永久阻塞了
}

/*
 * @brief 运行一次工作的处理
 * @description 处理完成就释放空间
 * @param work_node_t* 工作节点指针
 * @return void
 */
void work_singal_process(work_node_t *work_node) {
    work_node->work_func(work_node->arg);
    work_node_status_set(work_node, k_WORK_STATUS_DELETED);
}

/*
 * @brief：       运行循环工作处理
 * @description： 运行loop表中的工作节点
 * @param：       work_node_t* 工作节点指针
 * @return：      void
 */
void work_loop_process(work_node_t *node) {
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
void work_loop_check(work_node_t *node) {
    if (NULL == node) {
        return;
    }

    //todo 后期要换成内存池接口而不能耦合FreeRTOS
    if (k_WORK_STATUS_DELETED == node->status) {
        uxListRemove(&node->list_item);
        vPortFree(node);  // 释放工作节点
    }
    //todo 后期要换成内存池接口而不能耦合FreeRTOS
}

void work_node_status_set(work_node_t *node, e_work_loop_status_t status) { node->status = status; }