#include "serial.h"
#include "../log.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

#define SERIAL_REG_DATA 0x00
#define SERIAL_REG_STATUS 0x04
#define SERIAL_REG_CTRL 0x08

#define SERIAL_STATUS_RX_READY (1u << 0)
#define SERIAL_STATUS_TX_READY (1u << 1)

static int stdio_claimed;
static char claimed_files[SRA32KA_MAX_SERIAL][256];
static int claimed_file_count;

int serial_backend_open(serial_backend_t *backend, const char *spec)
{
    if (!spec || strcmp(spec, "null") == 0)
    {
        backend->kind = SERIAL_BACKEND_NULL;
        backend->file = NULL;
        return 0;
    }

    if (strcmp(spec, "stdio") == 0)
    {
        if (stdio_claimed)
            return SERIAL_BACKEND_ERR_BUSY;
        stdio_claimed = 1;
        backend->kind = SERIAL_BACKEND_STDIO;
        backend->file = NULL;
        return 0;
    }

    if (strncmp(spec, "file:", 5) == 0)
    {
        const char *path = spec + 5;
        FILE *f;
        int i;

        for (i = 0; i < claimed_file_count; i++)
            if (strcmp(claimed_files[i], path) == 0)
                return SERIAL_BACKEND_ERR_BUSY;

        f = fopen(path, "w");
        if (!f)
            return SERIAL_BACKEND_ERR_OPEN;

        if (claimed_file_count < SRA32KA_MAX_SERIAL)
            snprintf(claimed_files[claimed_file_count++], sizeof(claimed_files[0]), "%s", path);

        backend->kind = SERIAL_BACKEND_FILE;
        backend->file = f;
        return 0;
    }

    return SERIAL_BACKEND_ERR_UNKNOWN;
}

static struct termios old_term;
static int old_flags;
static int term_active;

static void stdio_backend_init(void)
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

static void stdio_backend_cleanup(void)
{
    if (!term_active)
        return;

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    fcntl(STDIN_FILENO, F_SETFL, old_flags);
    term_active = 0;
}

static void backend_putc(serial_backend_t *b, uint8_t c)
{
    switch (b->kind)
    {
    case SERIAL_BACKEND_STDIO:
        putchar(c);
        fflush(stdout);
        break;
    case SERIAL_BACKEND_FILE:
        fputc(c, b->file);
        fflush(b->file);
        break;
    case SERIAL_BACKEND_NULL:
        break;
    }
}

static int backend_poll_rx(serial_backend_t *b, uint8_t *out)
{
    unsigned char c;
    if (b->kind != SERIAL_BACKEND_STDIO)
        return 0;

    if (read(STDIN_FILENO, &c, 1) == 1)
    {
        *out = c;
        return 1;
    }
    return 0;
}

static void serial_poll_rx(serial_port_t *port)
{
    if (port->rx_ready)
        return;
    if (backend_poll_rx(&port->backend, &port->rx))
        port->rx_ready = 1;
}

static uint32_t serial_read(mmio_device_t *dev, uint32_t off, uint32_t size)
{
    (void)size;
    serial_port_t *port = dev->priv;

    switch (off)
    {
    case SERIAL_REG_DATA:
        port->rx_ready = 0;
        return port->rx;

    case SERIAL_REG_STATUS:
    {
        uint32_t status = 0;

        if (!port->enabled)
            return status;

        serial_poll_rx(port);
        if (port->rx_ready)
            status |= SERIAL_STATUS_RX_READY;
        status |= SERIAL_STATUS_TX_READY;
        return status;
    }

    case SERIAL_REG_CTRL:
        return port->enabled ? 1u : 0u;

    default:
        return 0;
    }
}

static void serial_write(mmio_device_t *dev, uint32_t off, uint32_t value, uint32_t size)
{
    (void)size;
    serial_port_t *port = dev->priv;

    switch (off)
    {
    case SERIAL_REG_DATA:
        if (port->enabled)
            backend_putc(&port->backend, (uint8_t)value);
        break;

    case SERIAL_REG_CTRL:
    {
        int enable = value & 1;
        if (enable && !port->enabled)
        {
            if (port->backend.kind == SERIAL_BACKEND_STDIO)
                stdio_backend_init();
            log_info("serial", "%s initialized", port->name);
        }
        port->enabled = enable;
        break;
    }

    default:
        break;
    }
}

void serial_port_init(mmio_device_t *dev, serial_port_t *port, uint32_t base, const char *name)
{
    dev->name = name;
    dev->base = base;
    dev->size = 0x0C;
    dev->priv = port;
    dev->read = serial_read;
    dev->write = serial_write;
    port->name = name;
    port->rx = 0;
    port->rx_ready = 0;
    port->enabled = 0;
    atexit(stdio_backend_cleanup);
}
