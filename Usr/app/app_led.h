#pragma once

#include "cmsis_os.h"
#include "ztask.h"

void app_led_init(void);
void app_led_scheduler_start(void);

osMessageQId get_led_msgq(void);