#include "work_timer.h"

#include "log/my_log.h"

/*
 * @description ����һ���ڶ�ʱ����������չ��һ��С��ʱ���� ��ʱ����������
 * @param xTimer ��ʱ����� ʹ�ýṹ���׵�ַ��ͬ���ص������ݲ���
 * @return ��
 * @note ��ʱ����������
 *
 */
void work_timer_process(TimerHandle_t xTimer) {
    /* ���ڶ�ʱ���������ܴ��ݲ���������ʹ�ýṹ��ͷ��ַ��ͬ�ķ��������д���*/  //?! ������
    work_timer_t *work_timer = (work_timer_t *)pvTimerGetTimerID(xTimer);
    work_timer_node_t *work_node = NULL;
    TickType_t xitemTicks = 0;
    UBaseType_t xItemNum = 0;
    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    if (listLIST_IS_EMPTY(&work_timer->work_time_list)) {
        LOGE("work_timer_process no work node");
        return;
    }

    for (work_node = (work_timer_node_t *)listGET_HEAD_ENTRY(&(work_timer->work_time_list)),
        xItemNum = listCURRENT_LIST_LENGTH(&(work_timer->work_time_list));
         xItemNum > 0;
         xItemNum--, work_node = (work_timer_node_t *)listGET_NEXT(&work_node->list_item)) {
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
 * @description ����һ����ʱ���ĳ�ʼ������
 * @param work_timer ��ʱ���ṹ��ָ��
 * @return ��
 * @note ��ʱ����ʼ������
 */
void work_timer_init(work_timer_t *work_timer) {
    vListInitialise(&work_timer->work_time_list);

    work_timer->work_timer =
        xTimerCreate("work_timer", 1, pdTRUE, (void *)work_timer, work_timer_process);
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
 * @description ����һ����ʱ����ɾ������
 * @param work_timer ��ʱ���ṹ��ָ��
 * @return ��
 * @note ��ʱ��ɾ������
 */
//! ע��: ǧ�����ö�ʱ���Լ�ɾ���Լ�����������  ɾ����ʱ����, ��ʱ��������ִ��
void work_timer_del(work_timer_t *work_timer) {
    xTimerDelete(work_timer->work_timer, 1000);  // ��ɾ����ʱ���ſ����ͷ�����
    work_timer_remove_all(work_timer);
    vSemaphoreDelete(work_timer->lock);
}

/*
 * @description ����һ����ʱ������������
 * @param work_timer ��ʱ���ṹ��ָ��
 *        xProcessTicks ��ʱ����������
 * @return ��
 * @note ��ʱ����������
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
 * @description ����һ����ʱ������ӹ����ڵ㺯��
 * @param work_timer ��ʱ���ṹ��ָ��
 *        id �����ڵ�id
 *        xItemValue �����ڵ㴦����
 *        work_func �����ڵ㴦����
 *        arg �����ڵ㴦��������
 * @return ��
 * @note ��ʱ����ӹ����ڵ㺯��
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
 * @description ����һ����ʱ����ɾ�������ڵ㺯��
 * @param work_timer ��ʱ���ṹ��ָ��
 *        id �����ڵ�id
 * @return ��
 * @note ��ʱ��ɾ�������ڵ㺯��
 */
void work_timer_node_remove(work_timer_t *work_timer, uint32_t id) {
    work_timer_node_t *work_node = NULL;
    UBaseType_t node_num = 0;
    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    for (work_node = (work_timer_node_t *)listGET_HEAD_ENTRY(&work_timer->work_time_list),
        node_num = listCURRENT_LIST_LENGTH(&work_timer->work_time_list);
         ((work_node->id != id) && node_num > 0);
         node_num--, work_node = (work_timer_node_t *)listGET_NEXT(&work_node->list_item)) {
    }
    if (node_num > 0) {
        uxListRemove(&work_node->list_item);
        vPortFree(work_node);  // �ͷŹ����ڵ�
    } else {
        LOGE("no exit this id %d,work_node is null\n", id);
    }
    xSemaphoreGive(work_timer->lock);
}

/*
 * @description ����һ����ʱ����ɾ�����й����ڵ㺯��
 * @param work_timer ��ʱ���ṹ��ָ��
 * @return ��
 * @note ��ʱ��ɾ�����й����ڵ㺯��
 */
void work_timer_remove_all(work_timer_t *work_timer) {
    work_timer_node_t *work_node = NULL;
    UBaseType_t node_num = 0;
    xSemaphoreTake(work_timer->lock, portMAX_DELAY);
    for (work_node = (work_timer_node_t *)listGET_HEAD_ENTRY(&work_timer->work_time_list),
        node_num = listCURRENT_LIST_LENGTH(&work_timer->work_time_list);
         node_num > 0;
         (node_num--, work_node = (work_timer_node_t *)listGET_NEXT(&work_node->list_item))) {
        uxListRemove(&work_node->list_item);
        vPortFree(work_node);  // �ͷŹ����ڵ�
    }
    xSemaphoreGive(work_timer->lock);
}
