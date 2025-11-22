# linux-wire

[![CI](https://img.shields.io/badge/CI-GitHub_Actions-6f42c1)](https://github.com/FEASTorg/linux-wire/actions/workflows/ci.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-20b2aa)](./LICENSE)
![Language](https://img.shields.io/badge/language-C%2B%2B%20%7C%20C-f39c12)
![CMake](https://img.shields.io/badge/build-CMake-708090)

linux-wire is a minimal Linux-native I²C abstraction library that mirrors Arduino’s `Wire` API. It ships a tiny C shim over `/dev/i2c-*` plus a `TwoWire` C++ class so Arduino-style sketches can run on Raspberry Pi–class systems without heavyweight dependencies.

## Quick Start

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Use `cmake --install build` to install headers and the static library; downstream CMake projects can simply `find_package(linux_wire CONFIG REQUIRED)` and link against `linux_wire::linux_wire`.

Example binaries (`i2c_scanner`, `master_reader`, `master_writer`) live under `build/` after compiling.

## Documentation

- [Docs index](./docs/index.md) — architecture overview, API examples, CI info.
- [Testing guide](./docs/testing.md) — running mock-based unit tests and suggested hardware validation steps.

## License

GPL-3.0-or-later. See [LICENSE](./LICENSE).
