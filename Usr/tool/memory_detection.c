#include "memory_detection.h"

#include "component.h"
#include "log/my_log.h"

static zThread_t memory_thread;

void memory_monitor_thread(void) {
    size_t xFreeHeapSize, minEverFreeHeapSize;
    LOGI("memory_monitor_thread start");

    osSignalWait(0X01, osWaitForever);
    xFreeHeapSize = xPortGetFreeHeapSize();
    minEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
    LOGI("free heap size: %d", xFreeHeapSize);
    LOGI("min ever free heap size: %d", minEverFreeHeapSize);
    osDelay(5);
}

static zThreadOS_t memory_thread_os;
void memory_monitor_thread_init(void) {
    // osThreadDef(memory_monitor, memory_monitor_thread, osPriorityAboveNormal, 0, 128);
    // memory_thread.id = osThreadCreate(osThread(memory_monitor), NULL);
    // if (!memory_thread.id) {
    //     LOGE("%s create error", (osThread(memory_monitor))->name);
    // }

    if (1 != zThread_create(&memory_thread_os, "Memory_monitor", memory_monitor_thread,
                            osPriorityNormal)) {
        LOGE("memory_monitor zThread_create error");
    }
}

osThreadId get_memory_moniter_thread_id(void) { return memory_thread.id; }

void check_memory(void) { osSignalSet(memory_thread.id, 0X01); }
