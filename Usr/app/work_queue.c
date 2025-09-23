#include "work_queue.h"

#include "log/my_log.h"

static void work_queue_remove(work_queue_t *work_queue);

/*
 * @brief ���������߳�
 *
 * @param void* �������в���ָ�룬��ʼ��ʱ����
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
                vPortFree(work_node);  // �ͷŹ����ڵ�
            }
        }
    }
}

/*
 * @brief ����һ����������
 *
 * @param work_queue* ��������ָ��
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
 * @brief ���һ������
 *
 * @param work_queue*               ��������ָ��
 * @param TickType_t xValue         �б���ֵ��ʱ(���ȼ�)
 * @param void (*work_func)(void *) ����ִ�к���
 * @param void *arg                 ����ִ�к�������
 */
void work_queue_add(work_queue_t *work_queue, TickType_t xValue, void (*work_func)(void *),
                    void *arg) {
    if (work_queue == NULL) {
        LOGE("work_queue_add work_queue is null");
        return;
    }

    work_node_t *work_node = pvPortMalloc(sizeof(work_node_t));  // ��̬�����б���

    if (work_node == NULL) {
        LOGE("work_queue_add malloc fail");
        return;
    }

    work_node->work_func = work_func;
    work_node->arg = arg;
    listSET_LIST_ITEM_VALUE(&work_node->list_item, xValue);

    xQueueSend(work_queue->work_queue, &work_node,
               portMAX_DELAY);  // ������������ӹ���,ʹ��0����Ҳ���Ǵ��ݵ�ַ�ķ����������ݴ���
}

/*
 * @brief ɾ��һ������
 *
 * @param work_queue* ��������ָ��
 */
void work_queue_remove(work_queue_t *work_queue) {
    if (work_queue == NULL) {
        LOGE("work_queue_remove work_queue is null");
        return;
    }

    uint32_t work_node_addr;
    xQueueReceive(work_queue->work_queue, &work_node_addr, portMAX_DELAY);  // ���չ����ڵ��ַ

    xSemaphoreTake(work_queue->lock, portMAX_DELAY);
    vListInsert(&work_queue->work_list, &(((work_node_t *)work_node_addr)->list_item));
    xSemaphoreGive(work_queue->lock);
}
