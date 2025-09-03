#include "my_log.h"

#include <stdarg.h>

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

void zlog(const char* fmt, ...) {
    int strlen = 0;
    char buff_str[128];

    /* rtos要用信号量锁住*/
    va_list args;
    va_start(args, fmt);
    strlen = vsprintf(buff_str, fmt, args);
    va_end(args);

    for (int i = 0; i < strlen; i++) {
        uint8_t c = buff_str[i];
        HAL_UART_Transmit(&huart1, (const uint8_t*)&c, 1, 100);
    }
}
