#include "Wire.h"

#include <cerrno>
#include <cstring>

TwoWire::TwoWire()
    : bus_open_(false),
      devicePath_{0},
      txAddress_(0),
      transmitting_(false),
      txBufferIndex_(0),
      txBufferLength_(0),
      rxBufferIndex_(0),
      rxBufferLength_(0),
      wireTimeoutUs_(0),
      wireTimeoutFlag_(false),
      wireResetOnTimeout_(false)
{
    bus_.fd = -1;
    bus_.device_path[0] = '\0';
    bus_.timeout_us = 0;
}

void TwoWire::begin(const char *device)
{
    if (!device || device[0] == '\0')
    {
        return;
    }

    if (bus_open_)
    {
        lw_close_bus(&bus_);
        bus_open_ = false;
    }

    std::strncpy(devicePath_, device, sizeof(devicePath_) - 1);
    devicePath_[sizeof(devicePath_) - 1] = '\0';

    if (lw_open_bus(&bus_, devicePath_) == 0)
    {
        bus_open_ = true;
    }
    else
    {
        bus_open_ = false;
    }

    resetTxBuffer();
    resetRxBuffer();
}

void TwoWire::begin(uint8_t /*address*/)
{
    // Slave mode is not supported on Linux user-space; no-op.
}

void TwoWire::begin(int address)
{
    begin(static_cast<uint8_t>(address));
}

void TwoWire::end()
{
    if (bus_open_)
    {
        lw_close_bus(&bus_);
        bus_open_ = false;
    }
    resetTxBuffer();
    resetRxBuffer();
}

void TwoWire::setClock(uint32_t /*frequency*/)
{
    // Not currently supported; bus frequency is typically set by kernel/DT.
}

void TwoWire::setWireTimeout(uint32_t timeout_us, bool reset_with_timeout)
{
    wireTimeoutUs_ = timeout_us;
    wireResetOnTimeout_ = reset_with_timeout;
    wireTimeoutFlag_ = false;

    // Store timeout in C core; currently informational.
    lw_set_timeout(&bus_, timeout_us);
}

bool TwoWire::getWireTimeoutFlag(void) const
{
    return wireTimeoutFlag_;
}

void TwoWire::clearWireTimeoutFlag(void)
{
    wireTimeoutFlag_ = false;
}

void TwoWire::beginTransmission(uint8_t address)
{
    transmitting_ = true;
    txAddress_ = address;
    resetTxBuffer();
}

void TwoWire::beginTransmission(int address)
{
    beginTransmission(static_cast<uint8_t>(address));
}

uint8_t TwoWire::endTransmission(uint8_t sendStop)
{
    if (!bus_open_)
    {
        return 4; // other error
    }

    if (!transmitting_)
    {
        return 4; // no active transmission
    }

    // Check buffer size vs capacity
    if (txBufferLength_ > LINUX_WIRE_BUFFER_LENGTH)
    {
        resetTxBuffer();
        transmitting_ = false;
        return 1; // data too long
    }

    // Select slave
    if (lw_set_slave(&bus_, txAddress_) != 0)
    {
        handleTimeoutFromErrno();
        resetTxBuffer();
        transmitting_ = false;
        return 4;
    }

    // By default we perform a write which issues a STOP. If a caller passed
    // sendStop==0 (repeated-start intent) we will not perform the write here
    // and leave the txBuffer intact so `requestFrom` can consume it in a
    // combined write-then-read ioctl call (repeated start).
    if (sendStop == 0)
    {
        // We just exit master transmit mode and keep buffer for a repeated-start
        transmitting_ = false;
        return 0;
    }

    // Normal write + STOP
    ssize_t written = lw_write(&bus_, txBuffer_, txBufferLength_, 1 /* send_stop */);
    transmitting_ = false;

    if (written < 0 || static_cast<std::size_t>(written) != txBufferLength_)
    {
        handleTimeoutFromErrno();
        resetTxBuffer();
        return 4;
    }

    resetTxBuffer();
    return 0; // success
}


uint8_t TwoWire::endTransmission(void)
{
    return endTransmission(1);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
    const uint8_t *internal = nullptr;
    std::size_t internalLen = 0;
    bool consumePendingTx = false;

    if (txBufferLength_ > 0)
    {
        if (txAddress_ == address)
        {
            internal = txBuffer_;
            internalLen = txBufferLength_;
            consumePendingTx = true;
        }
        else
        {
            resetTxBuffer();
        }
    }

    return requestFrom(address, quantity, internal, internalLen, sendStop, consumePendingTx);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
    return requestFrom(address, quantity, 1);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop)
{
    if (isize > INTERNAL_ADDRESS_MAX)
    {
        isize = INTERNAL_ADDRESS_MAX;
    }

    resetTxBuffer();

    if (isize == 0)
    {
        return requestFrom(address, quantity, nullptr, 0, sendStop, false);
    }

    uint8_t iaddr_buf[INTERNAL_ADDRESS_MAX] = {0};
    for (uint8_t i = 0; i < isize; ++i)
    {
        const uint8_t shift = (isize - 1U - i) * 8U;
        iaddr_buf[i] = static_cast<uint8_t>((iaddress >> shift) & 0xFF);
    }

    return requestFrom(address, quantity, iaddr_buf, isize, sendStop, false);
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
    if (quantity < 0)
    {
        return 0;
    }
    return requestFrom(static_cast<uint8_t>(address),
                       static_cast<uint8_t>(quantity),
                       1);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop)
{
    if (quantity < 0)
    {
        return 0;
    }
    return requestFrom(static_cast<uint8_t>(address),
                       static_cast<uint8_t>(quantity),
                       static_cast<uint8_t>(sendStop));
}

size_t TwoWire::write(uint8_t data)
{
    if (!transmitting_)
    {
        // On Arduino this could be "slave send mode"; here we do nothing.
        return 0;
    }

    if (txBufferLength_ >= LINUX_WIRE_BUFFER_LENGTH)
    {
        return 0;
    }

    txBuffer_[txBufferIndex_] = data;
    ++txBufferIndex_;
    txBufferLength_ = txBufferIndex_;

    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    if (!transmitting_ || !data || quantity == 0)
    {
        return 0;
    }

    size_t written = 0;
    for (size_t i = 0; i < quantity; ++i)
    {
        if (write(data[i]) == 0)
        {
            // Buffer full; stop.
            break;
        }
        ++written;
    }
    return written;
}

int TwoWire::available(void) const
{
    if (rxBufferLength_ < rxBufferIndex_)
    {
        return 0;
    }
    return static_cast<int>(rxBufferLength_ - rxBufferIndex_);
}

int TwoWire::read(void)
{
    if (rxBufferIndex_ >= rxBufferLength_)
    {
        return -1;
    }

    int value = rxBuffer_[rxBufferIndex_];
    ++rxBufferIndex_;
    return value;
}

int TwoWire::peek(void)
{
    if (rxBufferIndex_ >= rxBufferLength_)
    {
        return -1;
    }
    return rxBuffer_[rxBufferIndex_];
}

void TwoWire::flush(void)
{
    // No underlying hardware FIFO; nothing to do.
}

void TwoWire::resetTxBuffer()
{
    txBufferIndex_ = 0;
    txBufferLength_ = 0;
}

void TwoWire::resetRxBuffer()
{
    rxBufferIndex_ = 0;
    rxBufferLength_ = 0;
}

uint8_t TwoWire::requestFrom(uint8_t address,
                             uint8_t quantity,
                             const uint8_t *internalAddress,
                             std::size_t internalAddressLength,
                             uint8_t /*sendStop*/,
                             bool consumePendingTx)
{
    if (!bus_open_ || quantity == 0)
    {
        if (consumePendingTx)
        {
            resetTxBuffer();
        }
        return 0;
    }

    if (quantity > LINUX_WIRE_BUFFER_LENGTH)
    {
        quantity = LINUX_WIRE_BUFFER_LENGTH;
    }

    ssize_t result = 0;

    if (internalAddress && internalAddressLength > 0)
    {
        result = lw_ioctl_read(&bus_, address, internalAddress, internalAddressLength, rxBuffer_, quantity, 0);
    }
    else
    {
        if (lw_set_slave(&bus_, address) != 0)
        {
            handleTimeoutFromErrno();
            resetRxBuffer();
            if (consumePendingTx)
            {
                resetTxBuffer();
            }
            return 0;
        }

        result = lw_read(&bus_, rxBuffer_, quantity);
    }

    if (consumePendingTx)
    {
        resetTxBuffer();
    }

    if (result <= 0)
    {
        handleTimeoutFromErrno();
        resetRxBuffer();
        if (consumePendingTx)
        {
            resetTxBuffer();
        }
        return 0;
    }

    rxBufferIndex_ = 0;
    rxBufferLength_ = static_cast<std::size_t>(result);
    if (rxBufferLength_ > LINUX_WIRE_BUFFER_LENGTH)
    {
        rxBufferLength_ = LINUX_WIRE_BUFFER_LENGTH;
    }

    return static_cast<uint8_t>(rxBufferLength_);
}

void TwoWire::handleTimeoutFromErrno()
{
    if (wireTimeoutUs_ == 0 || errno != ETIMEDOUT)
    {
        return;
    }

    wireTimeoutFlag_ = true;
    if (wireResetOnTimeout_)
    {
        if (!reopenBus(devicePath_))
        {
            resetTxBuffer();
            resetRxBuffer();
        }
        else
        {
            resetTxBuffer();
            resetRxBuffer();
        }
    }
}

bool TwoWire::reopenBus(const char *device)
{
    if (!device || device[0] == '\0')
    {
        bus_open_ = false;
        return false;
    }

    lw_close_bus(&bus_);
    if (lw_open_bus(&bus_, device) == 0)
    {
        bus_open_ = true;
        return true;
    }

    bus_open_ = false;
    return false;
}

/* Global instance */
TwoWire Wire;
