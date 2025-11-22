#include "linux_wire.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

int lw_open_bus(lw_i2c_bus *bus, const char *device_path)
{
    if (!bus || !device_path || device_path[0] == '\0')
    {
        errno = EINVAL;
        return -1;
    }

    int fd = open(device_path, O_RDWR);
    if (fd < 0)
    {
        perror("lw_open_bus: open");
        bus->fd = -1;
        bus->device_path[0] = '\0';
        bus->timeout_us = 0;
        return -1;
    }

    bus->fd = fd;
    strncpy(bus->device_path, device_path, sizeof(bus->device_path) - 1);
    bus->device_path[sizeof(bus->device_path) - 1] = '\0';
    bus->timeout_us = 0;

    return 0;
}

void lw_close_bus(lw_i2c_bus *bus)
{
    if (!bus)
    {
        return;
    }
    if (bus->fd >= 0)
    {
        close(bus->fd);
        bus->fd = -1;
    }
    bus->device_path[0] = '\0';
    bus->timeout_us = 0;
}

int lw_set_slave(lw_i2c_bus *bus, uint8_t addr)
{
    if (!bus || bus->fd < 0)
    {
        errno = EBADF;
        return -1;
    }

    if (ioctl(bus->fd, I2C_SLAVE, addr) < 0)
    {
        perror("lw_set_slave: I2C_SLAVE");
        return -1;
    }

    return 0;
}

ssize_t lw_write(lw_i2c_bus *bus,
                 const uint8_t *data,
                 size_t len,
                 int send_stop)
{
    (void)send_stop; /* Currently ignored: each write issues a STOP */

    if (!bus || bus->fd < 0 || (!data && len > 0))
    {
        errno = EINVAL;
        return -1;
    }

    if (len == 0)
    {
        return 0;
    }

    ssize_t written = write(bus->fd, data, len);
    if (written < 0)
    {
        perror("lw_write: write");
    }
    return written;
}

ssize_t lw_read(lw_i2c_bus *bus,
                uint8_t *data,
                size_t len)
{
    if (!bus || bus->fd < 0 || !data)
    {
        errno = EINVAL;
        return -1;
    }

    if (len == 0)
    {
        return 0;
    }

    ssize_t r = read(bus->fd, data, len);
    if (r < 0)
    {
        perror("lw_read: read");
    }
    return r;
}

ssize_t lw_ioctl_read(lw_i2c_bus *bus,
                      uint16_t addr,
                      const uint8_t *iaddr,
                      size_t iaddr_len,
                      uint8_t *data,
                      size_t len,
                      uint16_t flags)
{
    if (!bus || bus->fd < 0 || !data || len == 0 || (iaddr_len > 0 && !iaddr))
    {
        errno = EINVAL;
        return -1;
    }

    if (iaddr_len > UINT16_MAX || len > UINT16_MAX)
    {
        errno = EINVAL;
        return -1;
    }

    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    memset(&msgs, 0, sizeof(msgs));
    memset(&rdwr, 0, sizeof(rdwr));

    int msg_count = 0;

    if (iaddr && iaddr_len > 0)
    {
        /* Write internal address first */
        msgs[msg_count].addr = addr;
        msgs[msg_count].flags = flags;
        /* i2c_msg.buf is non-const in kernel API */
        msgs[msg_count].buf = (uint8_t *)iaddr;
        msgs[msg_count].len = (uint16_t)iaddr_len;
        ++msg_count;
    }

    /* Read message */
    msgs[msg_count].addr = addr;
    msgs[msg_count].flags = flags | I2C_M_RD;
    msgs[msg_count].buf = data;
    msgs[msg_count].len = (uint16_t)len;
    ++msg_count;

    rdwr.msgs = msgs;
    rdwr.nmsgs = msg_count;

    if (ioctl(bus->fd, I2C_RDWR, &rdwr) < 0)
    {
        perror("lw_ioctl_read: I2C_RDWR");
        return -1;
    }

    return (ssize_t)len;
}

ssize_t lw_ioctl_write(lw_i2c_bus *bus,
                       uint16_t addr,
                       const uint8_t *iaddr,
                       size_t iaddr_len,
                       const uint8_t *data,
                       size_t len,
                       uint16_t flags)
{
    if (!bus || bus->fd < 0 ||
        (len > 0 && !data) ||
        (iaddr_len > 0 && !iaddr) ||
        (iaddr_len + len == 0))
    {
        errno = EINVAL;
        return -1;
    }

    if (iaddr_len > UINT16_MAX || len > UINT16_MAX)
    {
        errno = EINVAL;
        return -1;
    }

    /* Build a single write buffer: [iaddr (optional)] [data]
       Be cautious about size - a reasonable upper bound is applied here. */
    const size_t max_payload = 4096;
    if (iaddr_len + len > max_payload || iaddr_len + len > UINT16_MAX)
    {
        errno = EINVAL;
        return -1;
    }

    uint8_t *buf = (uint8_t *)malloc(iaddr_len + len);
    if (!buf)
    {
        errno = ENOMEM;
        return -1;
    }

    if (iaddr && iaddr_len > 0)
        memcpy(buf, iaddr, iaddr_len);
    if (data && len > 0)
        memcpy(buf + iaddr_len, data, len);

    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    memset(&msg, 0, sizeof(msg));
    memset(&rdwr, 0, sizeof(rdwr));

    msg.addr = addr;
    msg.flags = flags;
    msg.buf = buf;
    msg.len = (uint16_t)(iaddr_len + len);

    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;

    if (ioctl(bus->fd, I2C_RDWR, &rdwr) < 0)
    {
        perror("lw_ioctl_write: I2C_RDWR");
        free(buf);
        return -1;
    }

    free(buf);
    return (ssize_t)len;
}

int lw_set_timeout(lw_i2c_bus *bus, uint32_t timeout_us)
{
    if (!bus)
    {
        return -1;
    }
    bus->timeout_us = timeout_us;
    /* Currently no enforcement; placeholder for future poll/select logic. */
    return 0;
}
