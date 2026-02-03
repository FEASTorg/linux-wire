# I²C Slave Mode Investigation

**Status:** Not currently pursued - requires kernel driver development beyond library scope

---

## Background

Linux kernel I²C slave mode is **driver-based** rather than direct /dev/i2c-X access. The kernel provides an event-driven callback interface for I²C slave backends implemented as kernel drivers.

**Key Resources:**

- [Linux I²C Slave Interface Documentation](https://docs.kernel.org/i2c/slave-interface.html)
- [Linux I²C /dev Interface (master mode only)](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html)

---

## Architectural Constraint

Unlike I²C master mode (which userspace accesses via /dev/i2c-X), **slave mode requires kernel driver support**. Userspace cannot directly register as an I²C slave using standard ioctl interfaces.

### Kernel Requirements

- **CONFIG_I2C_SLAVE=y** in kernel build (check: `grep I2C_SLAVE /boot/config-$(uname -r)`)
- Raspberry Pi: bcm2835 driver supports slave mode since kernel 4.9+
- Hardware support varies by I²C bus controller

### Slave Mode Architecture

The kernel slave interface uses an event-based callback model:

- **Backend Driver:** Implements i2c_driver with `slave_callback` function
- **Events:** WRITE_REQUESTED, READ_REQUESTED, WRITE_RECEIVED, READ_PROCESSED, STOP
- **Registration:** `i2c_slave_register()` kernel function
- **Userspace Instantiation:** sysfs via `echo slave-<backend> 0x1<addr> > /sys/bus/i2c/devices/i2c-X/new_device`

**Example Backends:**

- `i2c-slave-eeprom`: Emulate EEPROM chip (24c02, etc.)
- Custom kernel modules for specific use cases

---

## Implementation Options Considered

### Option A: Use Existing Kernel Slave Backends

**Approach:** Provide helper library for instantiating/interacting with existing kernel slave backends via sysfs.

**Pros:**

- No kernel module development required
- Works with upstream kernel

**Cons:**

- Limited to functionality of existing backends (EEPROM emulation, etc.)
- Not suitable for CRUMBS protocol implementation
- No event callbacks to userspace

**Verdict:** Insufficient for CRUMBS requirements

### Option B: Develop Custom Kernel Module

**Approach:** Create CRUMBS-specific I²C slave kernel driver with userspace interface.

**Pros:**

- Full control over slave behavior
- Can implement CRUMBS protocol in kernel or bridge to userspace
- Proper event-driven architecture

**Cons:**

- Requires kernel module development/maintenance
- Out-of-tree kernel module complexity
- Platform-specific builds
- Kernel development expertise required
- Distribution/installation complexity

**Verdict:** Beyond scope of minimal userspace library

### Option C: I²C Slave via Bit-Banged GPIO (i2c-gpio-slave)

**Approach:** Software I²C slave implementation using GPIO interrupts.

**Pros:**

- Fully userspace (with kernel GPIO support)
- Portable across platforms

**Cons:**

- CPU intensive (interrupt-driven)
- Timing constraints difficult in userspace
- Lower reliability than hardware I²C slave
- Clock stretching limitations
- Not practical for production use

**Verdict:** Too unreliable for CRUMBS protocol

---

## Conclusion

**linux-wire remains focused on I²C master mode only.** Slave mode implementation would require:

1. **Kernel driver development** - Out of scope for userspace library
2. **Platform-specific builds** - Against portability goals
3. **Kernel expertise** - High barrier to entry for contributors

### Recommended Alternatives for Peripheral Mode

For projects requiring Linux as an I²C peripheral (like CRUMBS):

1. **USB CDC/ACM** - Serial-over-USB with standard Linux support
2. **UART/Serial** - Direct serial communication via GPIO
3. **SPI** - Higher throughput, Linux has better userspace support
4. **Ethernet/TCP** - For networked deployments
5. **Custom USB HID** - For specific device classes

### Removed Original API Design

The original roadmap proposed a direct /dev/i2c-X userspace API (`lw_slave_init()`, `poll()` event loop) which **does not match Linux kernel architecture**. The I2C_SLAVE ioctl registers the address but does not provide event notification to userspace—events are delivered to kernel drivers only.

This investigation document preserves the technical analysis for future reference.

---

## Future Research

A potential long-term research direction would be developing a **generic I²C slave bridge kernel module** that exposes slave events to userspace via character device (similar to how /dev/i2c-X exposes master mode). This would require:

- Kernel module that implements i2c_driver with slave_callback
- Character device interface (/dev/i2c-slave-X)
- ioctl interface for event notification (possibly via eventfd)
- Proper buffering and flow control
- Multi-device support

This remains theoretical and is not actively pursued.
