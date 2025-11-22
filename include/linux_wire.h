#ifndef LINUX_WIRE_C_CORE_H
#define LINUX_WIRE_C_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h> /* for ssize_t */

    /*
     * Simple I2C bus handle for /dev/i2c-*.
     * This is intentionally minimal for clarity and robustness.
     */
    typedef struct
    {
        int fd;               /* File descriptor for /dev/i2c-X (or -1 if closed) */
        char device_path[64]; /* Path used to open the bus, e.g. "/dev/i2c-1" */
        uint32_t timeout_us;  /* Reserved for future timeout behavior; 0 = no timeout */
    } lw_i2c_bus;

    /**
     * Open an I2C bus at device_path (e.g. "/dev/i2c-1").
     * On success, fills 'bus' and returns 0.
     * On failure, returns -1 and leaves 'bus->fd' == -1.
     */
    int lw_open_bus(lw_i2c_bus *bus, const char *device_path);

    /**
     * Close an I2C bus. Safe to call on an already-closed bus.
     */
    void lw_close_bus(lw_i2c_bus *bus);

    /**
     * Set the I2C slave address for subsequent operations.
     * Returns 0 on success, -1 on error.
     */
    int lw_set_slave(lw_i2c_bus *bus, uint8_t addr);

    /**
     * Write 'len' bytes from 'data' to the currently-selected slave.
     * 'send_stop' is currently accepted but ignored; each call issues a STOP.
     * Returns number of bytes written on success, or -1 on error.
     */
    ssize_t lw_write(lw_i2c_bus *bus,
                     const uint8_t *data,
                     size_t len,
                     int send_stop);

    /**
     * Read up to 'len' bytes into 'data' from the currently-selected slave.
     * Returns number of bytes read on success, or -1 on error.
     */
    ssize_t lw_read(lw_i2c_bus *bus,
                    uint8_t *data,
                    size_t len);

    /**
     * Perform a combined read that optionally sends an internal device address
     * (as a write message) followed by a read message in a single I2C_RDWR ioctl
     * transaction. This supports repeated-start style reads (no intervening STOP)
     * and is useful for reading device registers.
     *
     * Parameters:
     *  - addr: 7/10-bit device address
     *  - iaddr: pointer to internal address bytes to send before the read (may be NULL)
     *  - iaddr_len: number of internal address bytes (0 if none)
     *  - data: destination buffer for read data
     *  - len: number of bytes to read
     *  - flags: bitfield passed to i2c_msg.flags (allows I2C_M_TEN, I2C_M_IGNORE_NAK, etc.)
     */
    ssize_t lw_ioctl_read(lw_i2c_bus *bus,
                          uint16_t addr,
                          const uint8_t *iaddr,
                          size_t iaddr_len,
                          uint8_t *data,
                          size_t len,
                          uint16_t flags);

    /**
     * Perform a write using the I2C_RDWR ioctl. This allows writing an internal
     * register address and follow-up data in a single message and supports flags
     * (e.g. 10-bit addressing or ignoring NAK).
     */
    ssize_t lw_ioctl_write(lw_i2c_bus *bus,
                           uint16_t addr,
                           const uint8_t *iaddr,
                           size_t iaddr_len,
                           const uint8_t *data,
                           size_t len,
                           uint16_t flags);

    /**
     * Store a timeout value (in microseconds).
     * Currently this is informational only; no timeout enforcement is done.
     * Returns 0.
     */
    int lw_set_timeout(lw_i2c_bus *bus, uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif /* LINUX_WIRE_C_CORE_H */
