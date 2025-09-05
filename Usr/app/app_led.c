#include "app_led.h"

#include "cmsis_os.h"
#include "gpio.h"
#include "log/my_log.h"
#include "ztask.h"

static osThreadId app_led_handle = NULL;
void app_led_task(void* arg) {
    while (1) {
        LOGI("LED TASK");
        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(1000);
    }
}

void app_led_init(void) {
    /*hal_init*/
    osThreadDef(app_led, app_led_task, osPriorityAboveNormal, 0, 128);
    app_led_handle = osThreadCreate(osThread(app_led), NULL);
}
