#include "app_led.h"

#include "gpio.h"
#include "log/my_log.h"
static osThreadId app_led_handle = NULL;
static osMessageQId app_led_msgq_id = NULL;
static osEvent msg_event;
typedef void (*callback)(void);

static callback m_cb;

static void led_logic(void);
static void led_msg_process(void);

void app_led_task(void* arg) {
    while (1) {
        osSignalWait(0, osWaitForever);

        if (m_cb) m_cb();

        osDelay(10);
    }
}

void app_led_init(void) {
    /*hal_init*/
    osThreadDef(app_led, app_led_task, osPriorityAboveNormal, 0, 128);
    app_led_handle = osThreadCreate(osThread(app_led), NULL);

    /*msgq init*/
    osMessageQDef(app_led_msgq, 16, uint32_t);
    app_led_msgq_id = osMessageCreate(osMessageQ(app_led_msgq), NULL);

    set_callback(led_msg_process);
}

void app_led_scheduler_start(void) {
    if (app_led_handle != NULL) {
        osSignalSet(app_led_handle, 0x1);
    }
}

void set_callback(callback cb) { m_cb = cb; }

void led_logic(void) {
    while (1) {
        LOGI("LED TASK");
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);
    }
}

static uint8_t* buf_recv;
void led_msg_process(void) {
    while (1) {
        LOGI("msg TASK");
        msg_event = osMessageGet(app_led_msgq_id, osWaitForever);
        buf_recv = (uint8_t*)msg_event.value.v;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        LOGI("msg: %s\n", (char*)msg_event.value.v);
        LOGI("msg: %04X\n", msg_event.value.v);
        osDelay(500);
    }
}

osMessageQId get_led_msgq(void) { return app_led_msgq_id; }
