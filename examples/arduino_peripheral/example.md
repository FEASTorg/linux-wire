# Arduino Peripheral Companion Example

Use the following Arduino sketch to emulate a simple I²C slave that works with the linux-wire examples (`i2c_scanner`, `master_reader`, `master_writer`). Flash this onto an Arduino (e.g., Nano/Uno) and connect it to your Linux host via level shifters.

**Wiring**:

```sh
Raspberry Pi 3.3V  <— level shifter —>  Arduino 5V
Raspberry Pi SDA  <— level shifter —>  Arduino A4 (SDA)
Raspberry Pi SCL  <— level shifter —>  Arduino A5 (SCL)
Common GND
```

**How It Works**:

- `i2c_scanner` will detect the device at `0x40`.
- `master_writer` writes one byte to register 0, which the sketch stores in `registerValue`.
- `master_reader` first writes the register index (0x00) with `endTransmission(false)` and then reads one byte—the sketch responds with the current `registerValue`.
