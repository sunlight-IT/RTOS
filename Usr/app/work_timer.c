#include "work_timer.h"

#include "log/my_log.h"

/*
 * @description 这是一个在定时器任务上扩展的一个小定时处理 定时器任务处理函数
 * @param xTimer 定时器句柄 使用结构体首地址相同的特点来传递参数
 * @return 无
 * @note 定时器任务处理函数
 *
 */
void work_timer_process(TimerHandle_t xTimer) {
    /* 由于定时器的任务不能传递参数，所以使用结构体头地址相同的方法来进行传递*/  //?! 不可行
    work_timer_t *work_timer = (work_timer_t *)pvTimerGetTimerID(xTimer);
    work_timer_node_t *work_node = NULL;
    uint8_t node_num = 0;
    TickType_t xitemTicks;
    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    if (listLIST_IS_EMPTY(&work_timer->work_time_list)) {
        LOGE("work_timer_process no work node");
        return;
    }

    for (work_node = (work_timer_node_t *)listGET_HEAD_ENTRY(&(work_timer->work_time_list));
         work_node != NULL; work_node = (work_timer_node_t *)listGET_NEXT(&work_node->list_item)) {
        if (work_node->xPassTime > 0) {
            work_node->xPassTime--;
        } else if (0 == work_node->xPassTime) {
            work_node->xPassTime = work_node->list_item.xItemValue;
            work_node->work_func(work_node->arg);
        }
    }

    xSemaphoreGive(work_timer->lock);
}

/*
 * @description 这是一个定时器的初始化函数
 * @param work_timer 定时器结构体指针
 * @return 无
 * @note 定时器初始化函数
 */
void work_timer_init(work_timer_t *work_timer) {
    vListInitialise(&work_timer->work_time_list);

    work_timer->work_timer =
        xTimerCreate("work_timer", 1000, pdTRUE, (void *)work_timer, work_timer_process);
    if (work_timer->work_timer == NULL) {
        LOGE("work_timer_init error");
        return;
    }
    work_timer->lock = xSemaphoreCreateMutex();
    if (work_timer->lock == NULL) {
        LOGE("work_timer_init error");
        return;
    }
}

/*
 * @description 这是一个定时器的删除函数
 * @param work_timer 定时器结构体指针
 * @return 无
 * @note 定时器删除函数
 */
void work_timer_del(work_timer_t *work_timer) {
    work_timer_remove_all(work_timer);
    xTimerDelete(work_timer->work_timer, 1000);
    vSemaphoreDelete(work_timer->lock);
}

/*
 * @description 这是一个定时器的启动函数
 * @param work_timer 定时器结构体指针
 *        xProcessTicks 定时器任务处理间隔
 * @return 无
 * @note 定时器启动函数
 */
void work_timer_start(work_timer_t *work_timer, TickType_t xProcessTicks) {
    if (work_timer == NULL) {
        LOGE("work_timer_start:work_timer is null");
        return;
    }
    if (work_timer->work_timer == NULL) {
        LOGE("work_timer_start:work_timer->work_timer is null");
        return;
    }
    xTimerStart(work_timer->work_timer, xProcessTicks);
}

/*
 * @description 这是一个定时器的添加工作节点函数
 * @param work_timer 定时器结构体指针
 *        id 工作节点id
 *        xItemValue 工作节点处理间隔
 *        work_func 工作节点处理函数
 *        arg 工作节点处理函数参数
 * @return 无
 * @note 定时器添加工作节点函数
 */
void work_timer_node_add(work_timer_t *work_timer, uint32_t id, TickType_t xItemValue,
                         void (*work_func)(void *), void *arg) {
    work_timer_node_t *work_node = pvPortMalloc(sizeof(work_timer_node_t));
    if (work_node == NULL) {
        LOGE("work_node malloc fail");
        return;
    }
    work_node->work_func = work_func;
    work_node->arg = arg;
    work_node->xPassTime = xItemValue;
    work_node->id = id;
    listSET_LIST_ITEM_VALUE(&work_node->list_item, xItemValue);

    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    vListInsert(&work_timer->work_time_list, &work_node->list_item);
    xSemaphoreGive(work_timer->lock);
}

/*
 * @description 这是一个定时器的删除工作节点函数
 * @param work_timer 定时器结构体指针
 *        id 工作节点id
 * @return 无
 * @note 定时器删除工作节点函数
 */
void work_timer_node_remove(work_timer_t *work_timer, uint32_t id) {
    work_timer_node_t *work_node = NULL;
    uint8_t node_num = 0;
    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    for (work_node = (work_timer_node_t *)listGET_HEAD_ENTRY(&work_timer->work_time_list);
         (work_node != NULL && work_node->id != id);
         work_node = (work_timer_node_t *)listGET_NEXT(&work_node->list_item)) {
    }
    if (work_node == NULL) {
        LOGE("no exit this id %d,work_node is null\n", id);
    } else {
        uxListRemove(&work_node->list_item);
        vPortFree(work_node);  // 释放工作节点
    }
    xSemaphoreGive(work_timer->lock);
}

/*
 * @description 这是一个定时器的删除所有工作节点函数
 * @param work_timer 定时器结构体指针
 * @return 无
 * @note 定时器删除所有工作节点函数
 */
void work_timer_remove_all(work_timer_t *work_timer) {
    work_timer_node_t *work_node = NULL;
    uint8_t node_num = 0;
    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    for (work_node = (work_timer_node_t *)listGET_OWNER_OF_HEAD_ENTRY(&work_timer->work_time_list);
         work_node != NULL; work_node = (work_timer_node_t *)listGET_NEXT(&work_node->list_item)) {
        uxListRemove(&work_node->list_item);
        vPortFree(work_node);  // 释放工作节点
    }
    xSemaphoreGive(work_timer->lock);
}
