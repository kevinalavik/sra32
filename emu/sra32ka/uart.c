#include "uart.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdlib.h>

static struct termios old_term;
static int old_flags;
static int term_active = 0;

static void uart_terminal_init(void)
{
    if (term_active)
        return;

    struct termios term;
    tcgetattr(STDIN_FILENO, &old_term);
    term = old_term;
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK);
    term_active = 1;
}

static void uart_terminal_cleanup(void)
{
    if (!term_active)
        return;

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    fcntl(STDIN_FILENO, F_SETFL, old_flags);
    term_active = 0;
}

static void uart_poll_rx(uart_t *uart)
{
    if (uart->rx_ready)
        return;

    unsigned char c;

    ssize_t n = read(STDIN_FILENO, &c, 1);

    if (n == 1)
    {
        uart->rx = c;
        uart->rx_ready = 1;
    }
}

static uint32_t uart_read(mmio_device_t *dev, uint32_t off, uint32_t size)
{
    (void)size;

    uart_t *uart = dev->priv;

    switch (off)
    {
    case 0x00:
        uart->rx_ready = 0;
        return uart->rx;

    case 0x04:
    {
        uart_poll_rx(uart);

        uint32_t status = 0;

        if (uart->rx_ready)
            status |= (1 << 0); // RX ready
        status |= (1 << 1);     // TX ready
        return status;
    }

    default:
        return 0;
    }
}

static void uart_write(mmio_device_t *dev, uint32_t off, uint32_t value, uint32_t size)
{
    (void)dev;
    (void)size;

    switch (off)
    {
    case 0x00:
        putchar(value & 0xff);
        fflush(stdout);
        break;

    default:
        break;
    }
}

void uart_init(mmio_device_t *dev, uart_t *uart, uint32_t base)
{
    uart_terminal_init();

    dev->name = "uart0";
    dev->base = base;
    dev->size = 0x100;
    dev->priv = uart;
    dev->read = uart_read;
    dev->write = uart_write;

    uart->rx = 0;
    uart->rx_ready = 0;

    atexit(uart_terminal_cleanup);
}