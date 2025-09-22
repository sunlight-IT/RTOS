#pragma once
#include "cmsis_os.h"

/*
 * Work Queue
 * ��Ҫ�����ݺͷ�����
 * 1. ����
 * 2. �����߳�
 * 2. ������������
 * 3. ������/��Ƶ����ʹ���ź���
 * 4. ���߸�Ƶ����ʹ�ù�������
 *
 * ����
 * 1. ��ʼ��
 *      - ��������
 *      - �����ź���
 *      - ������������
 *
 * 2. ��ӹ���
 *       - ���������ڵ�
 *       - ��ӹ����ڵ㵽��������
 * 3. ���й���(����������)
 *       - �Ӷ�����ȡ��һ�������ڵ�
 *       - ���й����ڵ�
 * 4. ɾ������
 *       - �ӹ���������ɾ�������ڵ�
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
