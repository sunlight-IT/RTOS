# OSAL (Operating System Abstraction Layer)

## 项目简介

OSAL是一个为MCU设计的操作系统抽象层，提供统一的API接口，支持多种RTOS（如FreeRTOS、uC/OS-II、RT-Thread等），方便应用开发和移植。

## 特性

- **统一API接口**：提供task、queue、mutex、semaphore、event、memory等统一接口
- **多RTOS支持**：支持FreeRTOS、uC/OS-II、RT-Thread等主流RTOS
- **分层架构**：接口层、实现层、适配层三层架构
- **设计模式**：采用单例模式、工厂模式、策略模式
- **易于扩展**：方便添加新的RTOS支持

## 目录结构

```
OSAL/
├── inc/                      # 头文件目录
│   ├── osal.h               # OSAL主头文件
│   ├── osal_types.h         # 类型定义和错误码
│   ├── osal_config.h        # 配置文件
│   ├── osal_task.h          # Task组件接口
│   ├── osal_queue.h         # Queue组件接口
│   ├── osal_mutex.h         # Mutex组件接口
│   ├── osal_semaphore.h     # Semaphore组件接口
│   ├── osal_event.h         # Event组件接口
│   └── osal_memory.h        # Memory组件接口
├── src/                     # 源文件目录
│   ├── osal.c              # OSAL核心实现
│   ├── osal_ops.h          # 操作接口定义
│   ├── osal_task.c         # Task组件实现
│   ├── osal_queue.c        # Queue组件实现
│   ├── osal_sync.c         # 同步组件实现（Mutex/Semaphore/Event）
│   └── osal_memory.c       # Memory组件实现
├── port/                    # 适配层目录
│   ├── freertos/           # FreeRTOS适配层
│   ├── ucos_ii/            # uC/OS-II适配层
│   └── rtthread/           # RT-Thread适配层
└── examples/               # 示例代码
    ├── example_basic.c     # 基本使用示例
    ├── example_mutex.c     # 互斥锁示例
    ├── example_semaphore.c # 信号量示例
    └── example_event.c     # 事件示例
```

## 支持的RTOS

| RTOS | 状态 | 说明 |
|------|------|------|
| FreeRTOS | ? 完整支持 | 推荐使用 |
| uC/OS-II | ?? 部分支持 | 部分功能待完善 |
| RT-Thread | ?? 部分支持 | 部分功能待完善 |

## 快速开始

### 1. 配置OS类型

在 `osal_config.h` 中设置使用的OS类型：

```c
#define OSAL_OS_TYPE    OSAL_OS_FREERTOS
```

### 2. 初始化OSAL

```c
#include "osal.h"

int main(void)
{
    // 初始化OSAL
    osal_init(OSAL_OS_FREERTOS);

    // 创建任务...
    // 创建队列...

    // 启动调度器
    osal_start();

    return 0;
}
```

### 3. 创建任务

```c
void my_task(void *param)
{
    while (1) {
        // 任务逻辑
        osal_task_delay(1000);
    }
}

// 创建任务配置
osal_task_config_t config = {
    .name = "MyTask",
    .func = my_task,
    .param = NULL,
    .stack_size = 512,
    .priority = OSAL_PRIORITY_NORMAL,
    .time_slice = 0
};

osal_task_t handle;
osal_task_create(&config, &handle);
```

### 4. 使用队列

```c
// 创建队列
osal_queue_config_t queue_config = {
    .name = "MyQueue",
    .max_items = 10,
    .item_size = sizeof(int)
};
osal_queue_t queue;
osal_queue_create(&queue_config, &queue);

// 发送数据
int data = 100;
osal_queue_send(queue, &data, OSAL_WAIT_FOREVER);

// 接收数据
int received;
osal_queue_receive(queue, &received, OSAL_WAIT_FOREVER);
```

### 5. 使用互斥锁

```c
// 创建互斥锁
osal_mutex_config_t mutex_config = {
    .name = "MyMutex",
    .inherit = 1  // 启用优先级继承
};
osal_mutex_t mutex;
osal_mutex_create(&mutex_config, &mutex);

// 获取互斥锁
osal_mutex_acquire(mutex, OSAL_WAIT_FOREVER);

// 访问共享资源

// 释放互斥锁
osal_mutex_release(mutex);
```

## API参考

### 任务API

| API | 描述 |
|-----|------|
| `osal_task_create()` | 创建任务 |
| `osal_task_delete()` | 删除任务 |
| `osal_task_delay()` | 任务延时 |
| `osal_task_yield()` | 让出CPU |
| `osal_task_get_priority()` | 获取任务优先级 |
| `osal_task_set_priority()` | 设置任务优先级 |

### 队列API

| API | 描述 |
|-----|------|
| `osal_queue_create()` | 创建队列 |
| `osal_queue_send()` | 发送数据 |
| `osal_queue_receive()` | 接收数据 |
| `osal_queue_reset()` | 清空队列 |

### 同步API

| API | 描述 |
|-----|------|
| `osal_mutex_create()` | 创建互斥锁 |
| `osal_mutex_acquire()` | 获取互斥锁 |
| `osal_mutex_release()` | 释放互斥锁 |
| `osal_semaphore_create()` | 创建信号量 |
| `osal_semaphore_acquire()` | 获取信号量 |
| `osal_semaphore_release()` | 释放信号量 |
| `osal_event_create()` | 创建事件组 |
| `osal_event_wait()` | 等待事件 |
| `osal_event_set()` | 设置事件 |

## 配置选项

在 `osal_config.h` 中可以配置以下选项：

- `OSAL_OS_TYPE`：选择OS类型
- `OSAL_CFG_TASK_ENABLE`：启用任务模块
- `OSAL_CFG_QUEUE_ENABLE`：启用队列模块
- `OSAL_CFG_MUTEX_ENABLE`：启用互斥锁模块
- `OSAL_CFG_SEMAPHORE_ENABLE`：启用信号量模块
- `OSAL_CFG_EVENT_ENABLE`：启用事件模块
- `OSAL_CFG_MEMORY_ENABLE`：启用内存管理模块
- `OSAL_CFG_PARAM_CHECK`：启用参数检查
- `OSAL_CFG_DEBUG_OUTPUT`：启用调试输出

## 设计模式

### 单例模式

OSAL使用单例模式管理全局实例，确保整个系统中只有一个OSAL实例：

```c
typedef struct {
    osal_state_t             state;
    osal_os_type_t           os_type;
    const osal_port_desc_t   *port;
    // ...
} osal_instance_t;
```

### 工厂模式

OSAL使用工厂模式根据OS类型创建对应的适配层：

```c
const osal_port_desc_t *osal_get_port_desc(osal_os_type_t os_type)
{
    // 根据OS类型返回对应的适配层描述
}
```

### 策略模式

OSAL使用策略模式通过函数指针实现不同RTOS的操作接口：

```c
typedef struct {
    const osal_task_ops_t       *task;
    const osal_queue_ops_t      *queue;
    const osal_mutex_ops_t      *mutex;
    // ...
} osal_ops_t;
```

## 编译说明

### 使用CMake

```bash
mkdir build && cd build
cmake ..
make
```

### 手动编译

```bash
# 编译OSAL核心
gcc -c src/osal.c -I inc
gcc -c src/osal_task.c -I inc
gcc -c src/osal_queue.c -I inc
gcc -c src/osal_sync.c -I inc
gcc -c src/osal_memory.c -I inc

# 编译适配层（选择对应的RTOS）
gcc -c port/freertos/osal_port.c -I inc

# 链接
gcc -o my_app main.o osal.o osal_task.o osal_queue.o osal_sync.o osal_memory.o osal_port.o
```

## 移植指南

要添加新的RTOS支持，需要：

1. 在 `port/` 下创建新的目录
2. 实现 `osal_port.h` 和 `osal_port.c`
3. 在 `src/osal.c` 中添加新的操作接口
4. 在 `osal_config.h` 中添加新的OS类型

## 许可证

本项目仅供学习和参考使用。

## 贡献

欢迎提交Issue和Pull Request！

## 作者

Claude Code

## 版本历史

- **v1.0.0** (2024-01-20)
  - 初始版本
  - 支持FreeRTOS、uC/OS-II、RT-Thread
  - 实现task、queue、mutex、semaphore、event、memory组件
