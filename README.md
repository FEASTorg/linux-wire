# linux-wire

A minimal, Linux-native I²C library that replicates the core behavior and API style of Arduino’s `Wire` library.  
Provides:

- A small C backend over `/dev/i2c-*`
- A `TwoWire` C++ class (`Wire`) for simple, Arduino-like I²C master communication

This library is intended as a lightweight, foundational component for embedded Linux systems that need a familiar, buffer-based I²C API.

---

## Features

- Master-mode I²C read/write using Linux `i2c-dev`
- Arduino-style `Wire.begin()`, `Wire.beginTransmission()`, `Wire.write()`, `Wire.endTransmission()`, `Wire.requestFrom()`
- Register-friendly `requestFrom(address, quantity, iaddress, isize, sendStop)` helper that mirrors the upstream Arduino overload
- Internal RX/TX buffers modeled after Arduino `BUFFER_LENGTH`
- No external dependencies
- Simple CMake-based build

---

## Building

```sh
cmake -S . -B build
cmake --build build
```

---

## Installing System-Wide (Optional)

```sh
cmake --install build
sudo ldconfig
```

### Using linux-wire from another CMake project

```cmake
find_package(linux_wire CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE linux_wire::linux_wire)
```

The exported target exposes both the C and C++ headers, so including either `linux_wire.h` or `Wire.h` works without additional include path tweaks.

---

## Running Examples

```sh
cd build
./i2c_scanner
```

---

## Project Tree

```sh
linux-wire/
├── CMakeLists.txt
├── LICENSE
├── README.md
│
├── include/
│   ├── linux_wire.h      # C API
│   └── Wire.h            # C++ Wire-like API (TwoWire)
│
├── src/
│   ├── linux_wire.c      # C implementation using /dev/i2c-* and ioctl
│   ├── Wire.cpp          # TwoWire implementation using linux_wire.c
│   └── internal_config.h # buffer sizes, defaults (optional)
│
└── examples/
    ├── i2c_scanner/
    │   └── main.cpp
    ├── master_reader/
    │   └── main.cpp
    └── master_writer/
        └── main.cpp
```

---

## License

GNU General Public License v3.0 or later (GPL-3.0-or-later)
See [LICENSE](./LICENSE) for details.

---

## Notes on the Wire-style API

- The combined overload `Wire.requestFrom(address, quantity, iaddress, isize, sendStop)` is implemented so you can read internal registers without a manual write phase.
- Traditional repeated-start flows (`beginTransmission` → `write` → `endTransmission(false)` → `requestFrom`) are supported; the pending write bytes are buffered and consumed as part of a single `I2C_RDWR` transaction on the next `requestFrom` to that address. If you call `endTransmission(false)` without following up with `requestFrom`, those buffered bytes are flushed (with a STOP) before the next transmission to avoid silently discarding data.
- `Wire.setWireTimeout(timeout_us, reset_on_timeout)` tracks Linux `ETIMEDOUT` errors. If `reset_on_timeout` is true, the bus handle is closed and reopened automatically, matching the semantics of the upstream AVR implementation.
