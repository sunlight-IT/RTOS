#pragma once

typedef void (*task_func)(void *);

typedef struct {
    const char *name;
    task_func func;
} ztask_t;