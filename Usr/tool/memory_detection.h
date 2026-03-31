#pragma once

#include "cmsis_os2.h"

void memory_monitor_thread_init(void);
osThreadId_t get_memory_moniter_thread_id(void);
void check_memory(void);