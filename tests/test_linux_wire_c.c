#include "linux_wire.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define EXPECT_ERR(call, err)       \
    do                              \
    {                               \
        errno = 0;                  \
        assert((call) == -1);       \
        assert(errno == (err));     \
    } while (0)

int main(void)
{
    lw_i2c_bus bus;
    memset(&bus, 0, sizeof(bus));
    bus.fd = -1;
    bus.log_errors = 1;

    EXPECT_ERR(lw_open_bus(NULL, "/dev/null"), EINVAL);
    EXPECT_ERR(lw_open_bus(&bus, ""), EINVAL);

    EXPECT_ERR(lw_set_slave(&bus, 0x10), EBADF);

    uint8_t byte = 0x00;
    uint8_t buf[1];

    EXPECT_ERR(lw_write(&bus, &byte, 1, 1), EINVAL);
    EXPECT_ERR(lw_read(&bus, buf, 1), EINVAL);

    EXPECT_ERR(lw_ioctl_read(&bus, 0x10, &byte, 1, buf, 1, 0), EINVAL);

    bus.fd = 0; /* bypass fd check to hit other validation branches */

    EXPECT_ERR(lw_ioctl_write(&bus, 0x20, NULL, 1, &byte, 1, 0), EINVAL);
    EXPECT_ERR(lw_ioctl_write(&bus, 0x20, &byte, 1, NULL, 1, 0), EINVAL);
    EXPECT_ERR(lw_ioctl_write(&bus, 0x20, NULL, 0, NULL, 0, 0), EINVAL);

    lw_set_error_logging(&bus, 0);
    assert(bus.log_errors == 0);
    lw_set_error_logging(&bus, 1);
    assert(bus.log_errors == 1);

    return 0;
}
