/*
 * C example: i2c_scanner_strict
 * A stricter scanner that writes a dummy byte to ensure a real ACK on the data phase.
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

    /* Disable noisy perror logging while probing; many addresses will fail */
    lw_set_error_logging(&bus, 0);

    printf("Strict scanning I2C bus /dev/i2c-1...\n");

    uint8_t dummy = 0x00;
    for (int addr = 0x03; addr <= 0x77; ++addr)
    {
        if (lw_set_slave(&bus, (uint8_t)addr) != 0)
            continue;

        ssize_t w = lw_write(&bus, &dummy, 1, 1);
        if (w == 1)
        {
            printf("Found device at 0x%02X\n", addr);
        }
    }

    lw_close_bus(&bus);
    return 0;
}
