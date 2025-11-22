#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "Wire.h"
#include "mock_linux_wire.h"

static void testPlainReadUsesRead()
{
    mockLinuxWireReset();
    mockLinuxWireSetReadData({0x11, 0x22});

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    uint8_t count = tw.requestFrom(0x20, static_cast<uint8_t>(2));
    assert(count == 2);
    assert(tw.available() == 2);
    assert(tw.read() == 0x11);
    assert(tw.read() == 0x22);

    const auto &state = mockLinuxWireState();
    assert(state.readCalls == 1);
    assert(state.ioctlReadCalls == 0);

    tw.end();
}

static void testRepeatedStartUsesIoctl()
{
    mockLinuxWireReset();
    mockLinuxWireSetIoctlReadData({0xAB});

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    tw.beginTransmission(0x50);
    assert(tw.write(0x10) == 1);
    assert(tw.endTransmission(false) == 0);

    uint8_t count = tw.requestFrom(0x50, static_cast<uint8_t>(1));
    assert(count == 1);
    assert(tw.read() == 0xAB);

    const auto &state = mockLinuxWireState();
    assert(state.writeCalls == 0); // sendStop=false should defer write
    assert(state.ioctlReadCalls == 1);
    assert(state.lastIoctlAddr == 0x50);
    assert(state.lastIoctlInternal.size() == 1);
    assert(state.lastIoctlInternal[0] == 0x10);

    tw.end();
}

static void testInternalAddressClamp()
{
    mockLinuxWireReset();
    mockLinuxWireSetIoctlReadData({0x01, 0x02});

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    uint8_t count = tw.requestFrom(0x40,
                                   static_cast<uint8_t>(2),
                                   0x12345678,
                                   static_cast<uint8_t>(6),
                                   static_cast<uint8_t>(1));
    assert(count == 2);
    assert(tw.read() == 0x01);
    assert(tw.read() == 0x02);

    const auto &state = mockLinuxWireState();
    assert(state.ioctlReadCalls == 1);
    assert(state.lastIoctlInternal.size() == 4);
    assert(state.lastIoctlInternal[0] == 0x12);
    assert(state.lastIoctlInternal[1] == 0x34);
    assert(state.lastIoctlInternal[2] == 0x56);
    assert(state.lastIoctlInternal[3] == 0x78);

    tw.end();
}

static void testTimeoutFlagOnReadFailure()
{
    mockLinuxWireReset();
    mockLinuxWireForceReadError(ETIMEDOUT);

    TwoWire tw;
    tw.begin("/dev/i2c-mock");
    tw.setWireTimeout(1000, true);

    uint8_t count = tw.requestFrom(0x30, static_cast<uint8_t>(1));
    assert(count == 0);
    assert(tw.getWireTimeoutFlag());

    const auto &state = mockLinuxWireState();
    assert(state.openCalls == 2); // initial begin + reopen after timeout

    mockLinuxWireClearReadError();
    tw.end();
}

int main()
{
    testPlainReadUsesRead();
    testRepeatedStartUsesIoctl();
    testInternalAddressClamp();
    testTimeoutFlagOnReadFailure();

    std::puts("linux_wire tests passed");
    return 0;
}
