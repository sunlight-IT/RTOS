#include "my_log.h"

#include <stdarg.h>

#include "cmsis_os.h"
#include "core.h"
#include "usart.h"

// #ifdef __GNUC__
// #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
// #else
// #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
// #endif

// PUTCHAR_PROTOTYPE
// {
//   HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0x200);
//   return ch;
// }

static char s_buff_str[512];
static osMutexId s_log_mutex;

void zlog_init(void) {
    osMutexDef(LOG_MUTEX);
    s_log_mutex = osMutexCreate(osMutex(LOG_MUTEX));
}

void zlog(const char* fmt, ...) {
    int strlen = 0;
    // char buff_str[512];
    // char buff_str[256]; 会导致溢出，而且无法打印，64就可以正常打印
    /* rtos要用信号量锁住*/

#if _RTOS
    osMutexWait(s_log_mutex, osWaitForever);
#endif

    va_list args;
    va_start(args, fmt);
    strlen = vsprintf(s_buff_str, fmt, args);
    va_end(args);

    for (int i = 0; i < strlen; i++) {
        uint8_t c = s_buff_str[i];
        HAL_UART_Transmit(&huart1, (const uint8_t*)&c, 1, 100);
    }

#if _RTOS
    osMutexRelease(s_log_mutex);
#endif
}
