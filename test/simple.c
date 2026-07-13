#include <sra32ka/device.h>
#include <sra32ka/asm.h>

#define SERIAL_DATA 0x00
#define SERIAL_STATUS 0x04
#define SERIAL_CTRL 0x08
#define SERIAL_STATUS_TX_READY (1u << 1)

static void serial_putc(sra32ka_device_t *dev, uint32_t c)
{
    while (!(sra32ka_device_read32(dev, SERIAL_STATUS) & SERIAL_STATUS_TX_READY))
        ;
    sra32ka_device_write32(dev, SERIAL_DATA, c);
}

static void serial_puts(sra32ka_device_t *dev, const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            serial_putc(dev, '\r');
        serial_putc(dev, *s);
        s++;
    }
}

void _start(void)
{
    sra32ka_device_t serial0;

    if (sra32ka_device_open("sra32ka-host.serial.serial0", &serial0) == 0)
    {
        sra32ka_device_write32(&serial0, SERIAL_CTRL, 1);
        serial_puts(&serial0, "Hello, World!\n");
    }

    SRA32KA_HALT();
}
