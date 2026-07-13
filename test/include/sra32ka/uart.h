#ifndef SRA32KA_UART_H
#define SRA32KA_UART_H

#include <stdint.h>
#include <stddef.h>

/*
 * SRA32KA UART0 MMIO interface
 * UART0 registers:
 *  0x00 DATA
 *       Read  : receive byte
 *       Write : transmit byte
 *
 *  0x04 STATUS
 *       bit 0 : RX available
 *       bit 1 : TX available
 */

#define SRA32KA_UART0_BASE 0xFFFF0000

#define UART_DATA (*(volatile uint32_t *)(SRA32KA_UART0_BASE + 0x00))
#define UART_STATUS (*(volatile uint32_t *)(SRA32KA_UART0_BASE + 0x04))

#define UART_STATUS_RX_READY (1u << 0)
#define UART_STATUS_TX_READY (1u << 1)

static inline uint32_t uart_status(void)
{
    return UART_STATUS;
}

static inline int uart_rx_ready(void)
{
    return (UART_STATUS & UART_STATUS_RX_READY) != 0;
}

static inline int uart_tx_ready(void)
{
    return (UART_STATUS & UART_STATUS_TX_READY) != 0;
}

static inline void uart_putc(uint32_t c)
{
    while (!uart_tx_ready())
        ;

    UART_DATA = c;
}

static inline void uart_puts(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');

        uart_putc(*s);
        s++;
    }
}

static inline uint32_t uart_getc(void)
{
    while (!uart_rx_ready())
        ;

    return UART_DATA;
}

static inline void uart_gets(char *buf, size_t size)
{
    if (!buf || size == 0)
        return;

    size_t i = 0;

    while (i < size - 1)
    {
        uint32_t c = uart_getc();

        if (c == '\r' || c == '\n')
            break;

        buf[i] = c;
        i++;

        /* echo */
        uart_putc(c);
    }

    buf[i] = 0;

    uart_putc('\r');
    uart_putc('\n');
}

#endif // SRA32KA_UART_H