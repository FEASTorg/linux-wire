/*
 * C example: i2c_scanner
 * Mirrors examples/cpp/i2c_scanner/main.cpp but uses the C API (linux_wire.h)
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include "linux_wire.h"

int main(void)
{
    lw_i2c_bus bus;
    if (lw_open_bus(&bus, "/dev/i2c-1") != 0) {
        fprintf(stderr, "Failed to open /dev/i2c-1\n");
        return 1;
    }

    printf("Scanning I2C bus /dev/i2c-1...\n");

    uint8_t buf[1];
    for (int addr = 0x03; addr <= 0x77; ++addr)
    {
        /* Try a small read to detect a responsive device */
        ssize_t r = lw_ioctl_read(&bus, (uint16_t)addr, NULL, 0, buf, 1, 0);
        if (r == 1) {
            printf("Found device at 0x%02X\n", addr);
        }
    }

    lw_close_bus(&bus);
    return 0;
}
