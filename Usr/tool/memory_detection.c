#include "memory_detection.h"

#include "component.h"
#include "log/my_log.h"

static zThread_t memory_thread;

void memory_monitor_thread(const void* arg) {
    size_t xFreeHeapSize, minEverFreeHeapSize;

    while (1) {
        osSignalWait(0X01, osWaitForever);
        xFreeHeapSize = xPortGetFreeHeapSize();
        minEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
        LOGI("free heap size: %d", xFreeHeapSize);
        LOGI("min ever free heap size: %d", minEverFreeHeapSize);
        osDelay(5);
    }
}

void memory_monitor_thread_init(void) {
    osThreadDef(memory_monitor, memory_monitor_thread, osPriorityAboveNormal, 0, 128);
    memory_thread.id = osThreadCreate(osThread(memory_monitor), NULL);
    if (!memory_thread.id) {
        LOGE("%s create error", (osThread(memory_monitor))->name);
    }
}

osThreadId get_memory_moniter_thread_id(void) { return memory_thread.id; }

void check_memory(void) { osSignalSet(memory_thread.id, 0X01); }
