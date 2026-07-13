#include "sra32ka.h"
#include "mmio.h"
#include "uart.h"

#include <stdio.h>

void sra32ka_init(cpu_t *cpu)
{
    if (!cpu)
        return;
    static uart_t uart;
    static mmio_device_t uart_dev;

    uart_init(&uart_dev, &uart, 0xFFFF0000);
    if (mmio_register(&uart_dev) != 0)
    {
        fprintf(stderr, "[sra32ka] failed to register uart0\n");
        return;
    }
    fprintf(stderr, "[sra32ka] uart0 @ 0xFFFF0000\n");
}