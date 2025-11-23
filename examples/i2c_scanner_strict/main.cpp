/*
 * i2c_scanner_strict
 *
 * A stricter I²C scanner for Linux systems.
 *
 * Unlike the standard Linux scan (which performs a zero-byte write probe),
 * this version forces a *data phase* by writing one dummy byte before STOP.
 *
 * This avoids false positives from devices that ACK the address phase of
 * a write-only probe but would NACK when actual data is sent.  A common
 * example are AVR-based Arduino boards running Wire slave mode, whose TWI
 * hardware ACKs all addresses during quick-write probes.
 *
 * This scanner is therefore useful when:
 *   - you are scanning a bus that contains AVR/ATmega Wire slaves, or
 *   - you want to verify devices that may reject data-phase writes,
 *   - you want a “true ACK” that requires both address ACK + data write ACK.
 */

#include <cstdio>
#include "Wire.h"

int main()
{
    Wire.begin("/dev/i2c-1");

    printf("Strict scanning I2C bus /dev/i2c-1...\n");

    uint8_t dummy = 0x00;

    for (int address = 0x03; address <= 0x77; ++address)
    {
        Wire.beginTransmission(address);
        Wire.write(dummy);

        // Suppress the I/O error spam from strict scanning
        uint8_t error = Wire.endTransmission();

        if (error == 0)
        {
            printf("Found device at 0x%02X\n", address);
        }
    }

    return 0;
}