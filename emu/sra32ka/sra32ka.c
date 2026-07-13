#include "sra32ka.h"
#include "mmio.h"
#include "uart.h"
#include "../sra32.h"

#include <stdio.h>

void halt(cpu_t *cpu, const sra32_instr_t *in)
{
    (void)in;
    cpu->halted = 1;
}

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

    /* register sra32ka specific opcodes */
    sra32_register_op(0x20, "halt", INSTR_FORMAT_R, halt);
}