# Testing

## Unit Tests (Mocked)

The `tests/` directory contains host-only tests that mock the `lw_*` functions. These ensure buffer semantics, repeated-start flows, and timeout flags work as expected without hardware.

### Running locally

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests covered:

- Plain `requestFrom` using `lw_read`.
- Repeated-start reads that reuse the buffered TX bytes through `lw_ioctl_read`.
- Internal register helper (`requestFrom(addr, qty, iaddress, isize, sendStop)`).
- Timeout flag propagation when `lw_read` reports `ETIMEDOUT`.
- Deferred write flushing when `endTransmission(false)` isn't followed by a read.

## Hardware Tests

While the mock tests catch logic regressions, you should run hardware checks before tagging releases. Suggested workflow:

1. **Setup**: Attach an I²C peripheral to `/dev/i2c-1`. We provide a ready-made Arduino Nano sketch in [docs/example.md](examples.md) that acts as a simple slave at address `0x40`. Ensure `i2c-dev` is loaded (`sudo modprobe i2c-dev`).
2. **Scanner**: From the `build` directory, run `./i2c_scanner`. Confirm your device address appears.
3. **Writer**: Edit `examples/master_writer/main.cpp` with your device address/register, rebuild (`cmake --build build`), and run `./master_writer`.
4. **Reader / Repeated Start**: Use `examples/master_reader/main.cpp` to read back a register. It demonstrates `endTransmission(false)` + `requestFrom()` repeated-start flow.
5. **Custom Tests**: For more involved devices, embed the `linux_wire` library into your firmware/application and run device-specific validation.

## Continuous Integration

`.github/workflows/ci.yml` ensures every push/PR to `main` builds/tests on `ubuntu-22.04`. The workflow steps:

1. Checkout.
2. Install `cmake`, `ninja-build`, and `g++`.
3. Configure with `cmake -G Ninja -DBUILD_TESTING=ON`.
4. Build all targets.
5. Run `ctest`.

The CI log is a good indicator that the mock tests, examples, and packaging are intact. Always run hardware tests manually, as CI cannot access real devices.
