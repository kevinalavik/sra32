#ifndef UART_H
#define UART_H

#include "mmio.h"

typedef struct
{
    uint8_t rx;
    int rx_ready;
} uart_t;

void uart_init(mmio_device_t *dev, uart_t *uart, uint32_t base);

#endif // UART_H