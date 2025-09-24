#pragma once
#include <stdio.h>

#include "stm32f1xx.h"

int __io_putchar(int ch);
__attribute__((used)) int _write(int fd, char* buf, int size);

void zlog(const char* fmt, ...);

uint32_t zlog_get_tick(void);

#define LOGI(fmt, ...) \
    zlog("[%09lu] [INFO] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__)
#define LOGW(fmt, ...) \
    zlog("[%09lu] [WARN] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__)
#define LOGE(fmt, ...) \
    zlog("[%09lu] [EROR] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__)

#define vofa_LOGI(TYPE, fmt, ...) printf(" %s:" fmt "\n", TYPE, ##__VA_ARGS__)

void zlog_init(void);
