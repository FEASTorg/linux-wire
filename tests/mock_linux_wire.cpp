#include "mock_linux_wire.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

extern "C"
{
#include "linux_wire.h"
}

namespace
{
    struct MockConfig
    {
        std::vector<uint8_t> readData;
        std::vector<uint8_t> ioctlReadData;
        bool failRead = false;
        int failReadErrno = ETIMEDOUT;
    };

    MockLinuxWireState g_state;
    MockConfig g_config;
} // namespace

void mockLinuxWireReset()
{
    g_state = MockLinuxWireState{};
    g_config = MockConfig{};
}

void mockLinuxWireSetReadData(const std::vector<uint8_t> &data)
{
    g_config.readData = data;
}

void mockLinuxWireSetIoctlReadData(const std::vector<uint8_t> &data)
{
    g_config.ioctlReadData = data;
}

void mockLinuxWireForceReadError(int err)
{
    g_config.failRead = true;
    g_config.failReadErrno = err;
}

void mockLinuxWireClearReadError()
{
    g_config.failRead = false;
}

const MockLinuxWireState &mockLinuxWireState()
{
    return g_state;
}

extern "C"
{
    int lw_open_bus(lw_i2c_bus *bus, const char *device_path)
    {
        ++g_state.openCalls;
        if (!bus || !device_path || device_path[0] == '\0')
        {
            errno = EINVAL;
            return -1;
        }

        bus->fd = 1;
        std::strncpy(bus->device_path, device_path, LINUX_WIRE_DEVICE_PATH_MAX - 1);
        bus->device_path[LINUX_WIRE_DEVICE_PATH_MAX - 1] = '\0';
        bus->timeout_us = 0;
        g_state.lastDevicePath = device_path;
        return 0;
    }

    void lw_close_bus(lw_i2c_bus *bus)
    {
        ++g_state.closeCalls;
        if (bus)
        {
            bus->fd = -1;
            bus->device_path[0] = '\0';
        }
    }

    int lw_set_slave(lw_i2c_bus * /*bus*/, uint8_t addr)
    {
        ++g_state.setSlaveCalls;
        g_state.lastSetSlaveAddr = addr;
        return 0;
    }

    ssize_t lw_write(lw_i2c_bus * /*bus*/, const uint8_t *data, size_t len, int /*send_stop*/)
    {
        ++g_state.writeCalls;
        g_state.lastWriteBuffer.assign(data, data + len);
        return static_cast<ssize_t>(len);
    }

    ssize_t lw_read(lw_i2c_bus * /*bus*/, uint8_t *data, size_t len)
    {
        ++g_state.readCalls;
        if (g_config.failRead)
        {
            errno = g_config.failReadErrno;
            return -1;
        }

        const size_t to_copy = std::min(len, g_config.readData.size());
        if (to_copy > 0)
        {
            std::memcpy(data, g_config.readData.data(), to_copy);
        }
        else if (len > 0)
        {
            data[0] = 0;
        }
        g_state.lastReadBuffer.assign(data, data + to_copy);
        return static_cast<ssize_t>(to_copy);
    }

    ssize_t lw_ioctl_read(lw_i2c_bus * /*bus*/,
                          uint16_t addr,
                          const uint8_t *iaddr,
                          size_t iaddr_len,
                          uint8_t *data,
                          size_t len,
                          uint16_t /*flags*/)
    {
        ++g_state.ioctlReadCalls;
        g_state.lastIoctlAddr = addr;
        g_state.lastIoctlInternal.assign(iaddr, iaddr + iaddr_len);

        const size_t to_copy = std::min(len, g_config.ioctlReadData.size());
        if (to_copy > 0)
        {
            std::memcpy(data, g_config.ioctlReadData.data(), to_copy);
        }
        return static_cast<ssize_t>(to_copy);
    }

    ssize_t lw_ioctl_write(lw_i2c_bus * /*bus*/,
                           uint16_t addr,
                           const uint8_t *iaddr,
                           size_t iaddr_len,
                           const uint8_t *data,
                           size_t len,
                           uint16_t /*flags*/)
    {
        ++g_state.writeCalls;
        g_state.lastSetSlaveAddr = static_cast<uint8_t>(addr);
        g_state.lastWriteBuffer.assign(iaddr, iaddr + iaddr_len);
        g_state.lastWriteBuffer.insert(g_state.lastWriteBuffer.end(), data, data + len);
        return static_cast<ssize_t>(len);
    }

    int lw_set_timeout(lw_i2c_bus * /*bus*/, uint32_t timeout_us)
    {
        (void)timeout_us;
        return 0;
    }
} // extern "C"
