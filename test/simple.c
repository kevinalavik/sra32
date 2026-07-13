#include <sra32ka/uart.h>
#include <sra32ka/asm.h>

void _start(void)
{
    uart_puts("Hello, World!\n");
    SRA32KA_HALT();
}