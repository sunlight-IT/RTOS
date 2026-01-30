#pragma once

#include "cmsis_os.h"
#include "osal.h"

void memory_monitor_thread_init(void);
osal_task_t get_memory_moniter_thread_id(void);
void check_memory(void);