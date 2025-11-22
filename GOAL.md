# GOAL

We are building **linux-wire**, a minimal Linux-native library that replicates the core behavior and user-facing API of Arduino’s `Wire` (I²C) library. The goal is a simple, foundational, open-source I²C abstraction that feels like Arduino on embedded Linux systems such as Raspberry Pi. The design uses a clean C backend that wraps `/dev/i2c-*` and Linux `i2c-dev` ioctls, plus a lightweight C++ `TwoWire` class providing an Arduino-style interface (`Wire.begin()`, `Wire.beginTransmission()`, `Wire.write()`, `Wire.endTransmission()`, `Wire.requestFrom()`).

This library intentionally omits AVR internals, interrupts, and slave-mode behavior, focusing instead on portable, predictable master-mode communication with buffer semantics modeled on Arduino. The repo uses a simple CMake build, installs headers and a static library, and includes small example programs (`i2c_scanner`, `master_reader`, `master_writer`). The result is a stable, concise, and approachable I²C layer suitable for embedded Linux instruments, firmware-like applications, or higher-level abstraction libraries.

Key objectives:

- **Familiar API**: Replicate Arduino `Wire` API for easy adoption by Arduino users (see `_reference/Wire`)
- **Lightweight**: Minimal dependencies, small codebase, easy to build and integrate
- **Linux-native**: Use Linux `i2c-dev` interface for robust I²C communication (see `_reference/libi2c`)