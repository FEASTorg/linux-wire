#include <cstdio>
#include "Wire.h"

int main()
{
    Wire.begin("/dev/i2c-1");

    printf("Scanning I2C bus /dev/i2c-1...\n");

    for (int address = 0x03; address <= 0x77; ++address)
    {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0)
        {
            printf("Found device at 0x%02X\n", address);
        }
    }

    return 0;
}
