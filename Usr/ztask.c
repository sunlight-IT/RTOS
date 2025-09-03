#include "ztask.h"

#include "cmsis_os.h"

static task_func m_func_cb = NULL;

void APP_TASK_INIT(void) {
    osThreadDef(task_name, APP_TASK_func, osPriorityNormal, 1, 0);
    osThreadCreate(osThread(task_name), NULL);
}

void APP_TASK_func(void *arg) {
    if (m_func_cb) {
        m_func_cb(arg);
    }
}

void task_set_callback(task_func func) { m_func_cb = func; }