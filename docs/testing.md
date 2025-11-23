# Testing

## Unit Tests (Mocked)

The `tests/` directory contains host-only tests that replace the low-level `lw_*` functions with mocks. They exercise buffer semantics, repeated-start flows, timeout handling, and the strict scanner behavior.

### Running locally

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests covered:

- Plain `requestFrom` using `lw_read`
- Repeated-start reads via `lw_ioctl_read`
- Internal register helper (`requestFrom(addr, qty, iaddress, isize, sendStop)`)
- Timeout flag propagation when `lw_read` reports `ETIMEDOUT`
- Deferred write flushing when `endTransmission(false)` is not followed by a read
- Strict scanner write failures (logging suppressed in that binary)
- Basic negative-path checks for the C API (`lw_open_bus`, `lw_write`, `lw_ioctl_write`, etc.)

## Hardware Tests

Mock tests catch logic regressions, but you should still validate on real hardware before tagging releases:

1. **Setup**: Attach an I2C peripheral to `/dev/i2c-1`. You can use the Arduino companion sketch in [examples](./examples.md), which exposes a simple slave at `0x40`. Ensure `i2c-dev` is loaded (`sudo modprobe i2c-dev`).
2. **Scanner**: From `build/`, run `sudo ./i2c_scanner`. Confirm your peripheral shows up (the Arduino sketch will report at `0x40`).
3. **Writer**: Run `sudo ./master_writer` (or pass `--help` to adjust bus/address/register). Watch the Arduino serial log to confirm the byte was received.
4. **Reader / Repeated Start**: Run `sudo ./master_reader`. It performs `endTransmission(false)` followed by `requestFrom()`, verifying the repeated-start path.
5. **Extended exercise**: `sudo ./master_multiplier` writes a rolling pattern, then reads it back, providing a simple stress test.
6. **Custom tests**: Integrate `linux_wire` into your application and validate against the target device(s).

## Continuous Integration

`.github/workflows/ci.yml` runs on `ubuntu-22.04` for every push/PR targeting `main`. Steps:

1. Checkout
2. Install `cmake`, `ninja-build`, `gcc`, `g++`
3. `cmake -S . -B build -G Ninja -DBUILD_TESTING=ON`
4. `cmake --build build`
5. `ctest --test-dir build --output-on-failure`

CI ensures both the library and tests build cleanly on a baseline similar to Raspberry Pi OS. Always run the hardware checks above before releasing.***
