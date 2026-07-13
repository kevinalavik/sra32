#include "sra32ka.h"
#include "mmio.h"
#include "device.h"
#include "serial.h"
#include "../sra32.h"
#include "../log.h"

#include <stdio.h>

static serial_port_t serial_ports[SRA32KA_MAX_SERIAL];
static mmio_device_t serial_devs[SRA32KA_MAX_SERIAL];
static char serial_names[SRA32KA_MAX_SERIAL][8];

void halt(cpu_t *cpu, const sra32_instr_t *in)
{
    (void)in;
    cpu->halted = 1;
}

static const char *serial_backend_err(int rc)
{
    switch (rc)
    {
    case SERIAL_BACKEND_ERR_BUSY:
        return "endpoint already backs another serial port";
    case SERIAL_BACKEND_ERR_OPEN:
        return "could not open file";
    default:
        return "unknown backend";
    }
}

void sra32ka_init(cpu_t *cpu, const char *const *serial_specs, int serial_count)
{
    uint32_t host_id, serial_bus_id;
    int i, registered = 0;

    if (!cpu)
        return;

    if (serial_count < 0)
        serial_count = 0;
    if (serial_count > SRA32KA_MAX_SERIAL)
    {
        log_warn("sra32ka", "only %d serial ports supported, ignoring %d extra --serial option(s)",
                 SRA32KA_MAX_SERIAL, serial_count - SRA32KA_MAX_SERIAL);
        serial_count = SRA32KA_MAX_SERIAL;
    }

    device_tree_init();
    host_id = device_tree_add("sra32ka-host", DEVICE_NONE, 0);
    serial_bus_id = device_tree_add("serial", host_id, 0);

    for (i = 0; i < serial_count; i++)
    {
        uint32_t base = SRA32KA_SERIAL0_BASE + (uint32_t)i * SRA32KA_SERIAL_STRIDE;
        int rc;

        snprintf(serial_names[i], sizeof(serial_names[i]), "serial%d", i);

        rc = serial_backend_open(&serial_ports[i].backend, serial_specs[i]);
        if (rc != 0)
        {
            log_error("sra32ka", "%s: %s ('%s')", serial_names[i], serial_backend_err(rc), serial_specs[i]);
            continue;
        }

        serial_port_init(&serial_devs[i], &serial_ports[i], base, serial_names[i]);
        if (mmio_register(&serial_devs[i]) != 0)
        {
            log_error("sra32ka", "failed to register %s", serial_names[i]);
            continue;
        }

        device_tree_add(serial_names[i], serial_bus_id, base);
        log_info("sra32ka", "%s @ 0x%08X (%s, awaiting init)", serial_names[i], base, serial_specs[i]);
        registered++;
    }

    log_info("sra32ka", "device tree @ 0x%08X", DEVICE_TREE_BASE);

    /* register sra32ka specific opcodes */
    sra32_register_op(0x20, "halt", INSTR_FORMAT_R, halt);
}
