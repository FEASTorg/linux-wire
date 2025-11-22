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
- Internal RX/TX buffers modeled after Arduino `BUFFER_LENGTH`
- No external dependencies
- Simple CMake-based build

---

## Building

```sh
mkdir build
cd build
cmake ..
make
```

---

## Installing System-Wide (Optional)

```sh
sudo make install
sudo ldconfig
```

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
