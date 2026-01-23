#pragma once

#include "cmsis_os.h"

void memory_monitor_thread_init(void);
osThreadId get_memory_moniter_thread_id(void);
void check_memory(void);