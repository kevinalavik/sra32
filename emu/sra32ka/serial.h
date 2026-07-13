#ifndef SERIAL_H
#define SERIAL_H

#include "mmio.h"
#include <stdio.h>

#define SRA32KA_SERIAL0_BASE 0xFFFF0000
#define SRA32KA_SERIAL_STRIDE 0x1000
#define SRA32KA_MAX_SERIAL 8

typedef enum
{
    SERIAL_BACKEND_NULL,
    SERIAL_BACKEND_STDIO,
    SERIAL_BACKEND_FILE,
} serial_backend_kind_t;

typedef struct
{
    serial_backend_kind_t kind;
    FILE *file; // SERIAL_BACKEND_FILE only
} serial_backend_t;

#define SERIAL_BACKEND_ERR_UNKNOWN (-1) // not "stdio", "file:...", or "null"
#define SERIAL_BACKEND_ERR_BUSY (-2)    // endpoint already backs another port
#define SERIAL_BACKEND_ERR_OPEN (-3)    // file endpoint could not be opened

/* parses a stuff like "stdio", "file:path" or "null" returns 0 and fills backend on success, or one of the SERIAL_BACKEND_ERR_* codes */
int serial_backend_open(serial_backend_t *backend, const char *spec);

/*
 * a small serial port with word spaced registers, relative to the ports own base:
 *
 *   0x00 DATA    rw   read = rx byte, write = tx byte
 *   0x04 STATUS  ro   bit 0 = rx ready, bit 1 = tx ready
 *   0x08 CTRL    rw   bit 0 = ENABLE
 *
 * DATA/STATUS are dead, status reads 0, data writes are dropped until the guest writes CTRL.ENABLE.
 */
typedef struct
{
    serial_backend_t backend;
    const char *name;
    uint8_t rx;
    int rx_ready;
    int enabled;
} serial_port_t;

void serial_port_init(mmio_device_t *dev, serial_port_t *port, uint32_t base, const char *name);

#endif // SERIAL_H
