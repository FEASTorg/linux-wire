# linux-wire Documentation

## Overview

`linux-wire` is a minimal Linux-native implementation of Arduino's `Wire` API. The goal is to provide a tiny C backend over `/dev/i2c-*` and a drop-in `TwoWire` C++ wrapper so Raspberry Pi-class systems can reuse Arduino-style sketches without pulling in heavy dependencies. The library is master-mode only and mirrors Arduino buffer semantics (`BUFFER_LENGTH = 32`).

Key design points:

- C shim (`linux_wire.h` / `src/linux_wire.c`) handles `open`/`ioctl` interactions with `i2c-dev`.
- C++ `TwoWire` class (`Wire.h` / `Wire.cpp`) forwards user-facing Arduino methods to the C shim.
- Repeated-start reads are implemented via a combined `I2C_RDWR` ioctl so register reads behave like the AVR Wire reference.
- Internal TX/RX buffers stay fixed-size for deterministic behavior.

For a quick summary and project tree, see [README.md](../README.md).

## Building

Run the configuration and build steps from the project root:

```sh
# standard build:
cmake -S . -B build
# with tests:
cmake -S . -B build -DBUILD_TESTING=ON
# or with examples:
cmake -S . -B build -DLINUX_WIRE_BUILD_EXAMPLES=ON
# then build:
cmake --build build
# optionally install:
cmake --install build --prefix /desired/install/path
# clean build files:
cmake --build build --target clean
```

Notes:

- The project ships a `linux_wire` static library plus three example executables: `i2c_scanner`, `master_reader`, and `master_writer`.
- On Linux, no additional link libraries are typically needed beyond `pthread`/`rt` provided by the toolchain.
- `cmake --install build --prefix <dest>` installs headers into `<dest>/include` and exports a `linux_wire::linux_wire` CMake target so downstream projects can simply `find_package(linux_wire CONFIG REQUIRED)`.

## Running Examples

After building, the example binaries live under `build/`:

```sh
# Scan for devices on /dev/i2c-1:
sudo ./build/i2c_scanner /dev/i2c-1
# Read 2 bytes from register 0x00 of device at 0x40:
sudo ./build/master_reader /dev/i2c-1 0x40 0x00 2
# Write 3 bytes (0x01, 0x02, 0x03) to register 0x00 of device at 0x40:
sudo ./build/master_writer /dev/i2c-1 0x40 0x00 0x01 0x02 0x03
```

## Usage Guide

### C API

```c
#include "linux_wire.h"

lw_i2c_bus bus;
lw_open_bus(&bus, "/dev/i2c-1");
lw_set_slave(&bus, 0x40);
uint8_t value = 0xAB;
lw_write(&bus, &value, 1, 1);
lw_close_bus(&bus);
```

The C API exposes `lw_open_bus`, `lw_set_slave`, `lw_write`, `lw_read`, `lw_ioctl_read`, and `lw_ioctl_write`. These match the semantics of `_reference/libi2c`, but the surface area stays intentionally small for clarity. All functions validate buffers and return `-1` with `errno` set on failure.

### C++ `TwoWire`

```cpp
#include "Wire.h"

int main() {
    Wire.begin("/dev/i2c-1");
    Wire.beginTransmission(0x40);
    Wire.write(0x00); // register
    Wire.endTransmission(false); // hold the bus
    Wire.requestFrom(0x40, static_cast<uint8_t>(2)); // combined ioctl read
    while (Wire.available()) {
        int byte = Wire.read();
        // ...
    }
    Wire.end();
}
```

Important Arduino-parity notes:

- `requestFrom(address, quantity, iaddress, isize, sendStop)` exists and clamps `isize` to 4 bytes, mirroring the upstream Wire overload.
- Calling `endTransmission(false)` leaves the TX buffer intact for the next `requestFrom` to the same address. If no read follows, the buffer is flushed with a STOP before the next transmission to avoid silently dropping data.
- `setWireTimeout(timeout_us, reset_on_timeout)` sets a flag when Linux reports `ETIMEDOUT`. If `reset_on_timeout` is true, the bus handle is closed and reopened automatically (matching the AVR `twi` reset behavior).

## Testing

Software-only tests live in `tests/`. They use a mock `lw_*` backend to exercise buffer management, repeated-start logic, and timeout handling without real hardware:

```sh
ctest --test-dir build --output-on-failure
```

For hardware validation (recommended before release):

1. Connect a known I²C device to `/dev/i2c-1` (Raspberry Pi default).
2. Run `./build/i2c_scanner` to ensure the device shows up.
3. Adapt `examples/master_reader` or `examples/master_writer` with the target address/registers and confirm reads/writes succeed.

See [docs/testing.md](./testing.md) for detailed steps.

## CI

GitHub Actions runs on `ubuntu-22.04` (closest to Raspberry Pi OS Bookworm). The workflow installs CMake/Ninja/g++ and executes the build + test steps automatically on every push/PR targeting `main`.

## License

`linux-wire` is GPL-3.0-or-later. See [LICENSE](../LICENSE) for details.
