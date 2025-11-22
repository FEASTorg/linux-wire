#ifndef LINUX_WIRE_CPP_WIRE_H
#define LINUX_WIRE_CPP_WIRE_H

#include <cstddef>
#include <cstdint>

#include "linux_wire.h"

/**
 * Buffer size, mirroring Arduino's default BUFFER_LENGTH (32).
 * You can adjust this at compile time if needed.
 */
#ifndef LINUX_WIRE_BUFFER_LENGTH
#define LINUX_WIRE_BUFFER_LENGTH 32
#endif

/**
 * A minimal, Arduino-like TwoWire implementation for Linux.
 *
 * Differences vs AVR Wire:
 *  - Master mode only (no slave callbacks).
 *  - No inheritance from Stream / Print.
 *  - setClock() is currently a no-op (Linux bus speed is configured elsewhere).
 *  - endTransmission(sendStop) ignores sendStop; repeated starts are not yet supported.
 */
class TwoWire
{
public:
    static constexpr std::size_t INTERNAL_ADDRESS_MAX = 4;

    TwoWire();

    /**
     * Initialize on a specific I2C device path.
     * Default: "/dev/i2c-1".
     */
    void begin(const char *device = "/dev/i2c-1");

    /**
     * Arduino-style overloads for compatibility.
     * On Linux they are no-ops or unsupported.
     */
    void begin(uint8_t address); // slave mode - not supported, no-op
    void begin(int address);     // slave mode - not supported, no-op

    /**
     * Close the underlying I2C bus.
     */
    void end();

    /**
     * Set I2C bus speed (Hz). Currently a no-op; Linux bus speed is
     * typically configured via device tree / overlays.
     */
    void setClock(uint32_t frequency);

    /**
     * Timeout configuration (mirrors Arduino API shape, but currently
     * only stores the value).
     */
    void setWireTimeout(uint32_t timeout_us = 25000, bool reset_with_timeout = false);
    bool getWireTimeoutFlag(void) const;
    void clearWireTimeoutFlag(void);

    /**
     * Begin a master transmission to 'address'.
     */
    void beginTransmission(uint8_t address);
    void beginTransmission(int address);

    /**
     * End a transmission and actually write the TX buffer to the bus.
     *
     * Return codes are modeled after Arduino Wire:
     *  0: success
     *  1: data too long for buffer
     *  4: other error
     *
     * 'sendStop' is currently accepted but ignored; every transmission ends with a STOP.
     */
    uint8_t endTransmission(uint8_t sendStop);
    uint8_t endTransmission(void);

    /**
     * Request 'quantity' bytes from the given address, optionally specifying 'sendStop'.
     * Returns number of bytes actually read (0 on error).
     */
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop);
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop);
    uint8_t requestFrom(uint8_t address, uint8_t quantity);
    uint8_t requestFrom(int address, int quantity);
    uint8_t requestFrom(int address, int quantity, int sendStop);

    /**
     * Write data into the TX buffer.
     * Must be called after beginTransmission() and before endTransmission().
     */
    size_t write(uint8_t data);
    size_t write(const uint8_t *data, size_t quantity);
    size_t write(const char *str);

    /**
     * RX buffer accessors: must be called after requestFrom().
     */
    int available(void) const;
    int read(void);
    int peek(void);
    void flush(void);

private:
    lw_i2c_bus bus_;
    bool bus_open_;

    char devicePath_[LINUX_WIRE_DEVICE_PATH_MAX];
    uint8_t txAddress_;
    bool transmitting_;
    bool hasPendingTxForRead_;

    uint8_t txBuffer_[LINUX_WIRE_BUFFER_LENGTH];
    std::size_t txBufferIndex_;
    std::size_t txBufferLength_;

    uint8_t rxBuffer_[LINUX_WIRE_BUFFER_LENGTH];
    std::size_t rxBufferIndex_;
    std::size_t rxBufferLength_;

    uint32_t wireTimeoutUs_;
    bool wireTimeoutFlag_;
    bool wireResetOnTimeout_;

    void resetTxBuffer();
    void resetRxBuffer();
    uint8_t requestFrom(uint8_t address,
                        uint8_t quantity,
                        const uint8_t *internalAddress,
                        std::size_t internalAddressLength,
                        uint8_t sendStop,
                        bool consumePendingTx);
    void handleTimeoutFromErrno();
    bool reopenBus(const char *device);
    bool flushPendingRepeatedStart();
};

/* Global instance, as on Arduino */
extern TwoWire Wire;

#endif /* LINUX_WIRE_CPP_WIRE_H */
