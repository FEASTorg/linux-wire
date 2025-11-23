# Examples

## Arduino Peripheral Companion Example

Use this sketch to emulate a simple I²C slave that works with the `linux-wire` examples (`i2c_scanner`, `master_reader`, `master_writer`, `master_multiplier`). Flash it onto an Arduino (Nano/Uno) and connect it to your Raspberry Pi via a bidirectional level shifter.

### Wiring

| Raspberry Pi (3.3V) | Level Shifter | Arduino (5V) |
| ------------------- | ------------- | ------------ |
| 3.3V                | HV 5V         | 5V           |
| SDA (GPIO 2)        | SDA           | A4           |
| SCL (GPIO 3)        | SCL           | A5           |
| GND                 | GND           | GND          |

Keep the grounds common across the Pi, shifter, and Arduino.

### Sketch

```cpp
#include <Wire.h>

static const uint8_t DEVICE_ADDRESS = 0x40;
static volatile uint8_t registerValue = 0x00;

void receiveEvent(int numBytes)
{
    if (numBytes >= 1)
    {
        registerValue = Wire.read();

        while (Wire.available())
        {
            Wire.read();
        }
    }
}

void requestEvent()
{
    Wire.write(registerValue);
}

void setup()
{
    Wire.begin(DEVICE_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop()
{
    // all work handled in the Wire callbacks
}
```

This builds the same behavior that the Arduino `Wire` examples use: whatever byte the master last wrote will be returned on the next read.

## Test Flow

1. Flash the sketch and connect the Nano to the Pi via the level shifter.
2. On the Pi (with `linux-wire` built):

   ```sh
   cd build
   sudo ./i2c_scanner            # should report the device at 0x40
   sudo ./master_writer          # writes a test pattern to the Nano
   sudo ./master_reader          # reads it back (check serial monitor on the Nano to confirm activity)
   sudo ./master_multiplier      # optional: writes and reads a rolling pattern
   ```

   Each binary accepts optional command-line arguments (`./master_writer --help`) if you need to change bus name or address.

3. Use the Arduino Serial Monitor at 115200 baud to watch the `master_writer` / `master_reader` interaction (the example sketch prints the values it sees).

This setup gives you a deterministic loopback peripheral for debugging and CI smoke tests without relying on external sensors.\*\*\*
