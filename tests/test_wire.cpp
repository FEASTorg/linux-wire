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

    uint8_t count = tw.requestFrom(static_cast<uint8_t>(0x20), static_cast<uint8_t>(2));
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

    uint8_t count = tw.requestFrom(static_cast<uint8_t>(0x50), static_cast<uint8_t>(1));
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

    uint8_t count = tw.requestFrom(static_cast<uint8_t>(0x40),
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

    uint8_t count = tw.requestFrom(static_cast<uint8_t>(0x30), static_cast<uint8_t>(1));
    assert(count == 0);
    assert(tw.getWireTimeoutFlag());

    const auto &state = mockLinuxWireState();
    assert(state.openCalls == 2); // initial begin + reopen after timeout

    mockLinuxWireClearReadError();
    tw.end();
}

static void testDeferredWriteFlushes()
{
    mockLinuxWireReset();

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    tw.beginTransmission(static_cast<uint8_t>(0x22));
    tw.write(static_cast<uint8_t>(0x55));
    assert(tw.endTransmission(false) == 0);

    // No requestFrom; starting a new transmission should flush pending data.
    tw.beginTransmission(static_cast<uint8_t>(0x33));

    const auto &state = mockLinuxWireState();
    assert(state.writeCalls == 1);
    assert(state.lastSetSlaveAddr == 0x22);
    assert(state.lastWriteBuffer.size() == 1);
    assert(state.lastWriteBuffer[0] == 0x55);

    tw.endTransmission();
    tw.end();
}

static void testTxBufferOverflow()
{
    mockLinuxWireReset();

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    tw.beginTransmission(static_cast<uint8_t>(0x40));
    for (int i = 0; i < LINUX_WIRE_BUFFER_LENGTH; ++i)
    {
        assert(tw.write(static_cast<uint8_t>(i)) == 1);
    }
    // One more byte should be rejected.
    assert(tw.write(0xFF) == 0);
    assert(tw.endTransmission() == 0);

    tw.beginTransmission(static_cast<uint8_t>(0x40));
    for (int i = 0; i < LINUX_WIRE_BUFFER_LENGTH + 1; ++i)
    {
        tw.write(static_cast<uint8_t>(i));
    }
    assert(tw.endTransmission() == 1);

    tw.end();
}

static void testFlushOnDifferentAddress()
{
    mockLinuxWireReset();

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    tw.beginTransmission(static_cast<uint8_t>(0x10));
    tw.write(static_cast<uint8_t>(0xAA));
    assert(tw.endTransmission(false) == 0);

    // Requesting from a different address should flush the pending write.
    tw.requestFrom(static_cast<uint8_t>(0x20), static_cast<uint8_t>(1));

    const auto &state = mockLinuxWireState();
    assert(state.writeCalls == 1);
    assert(!state.lastWriteWasIoctl);
    assert(state.lastSetSlaveAddr == 0x10);
    assert(state.lastWriteBuffer.size() == 1);
    assert(state.lastWriteBuffer[0] == 0xAA);

    tw.end();
}

static void testZeroInternalAddressFallback()
{
    mockLinuxWireReset();
    mockLinuxWireSetReadData({0x99});

    TwoWire tw;
    tw.begin("/dev/i2c-mock");

    uint8_t count = tw.requestFrom(static_cast<uint8_t>(0x33),
                                   static_cast<uint8_t>(1),
                                   0,
                                   static_cast<uint8_t>(0),
                                   static_cast<uint8_t>(1));
    assert(count == 1);
    assert(tw.read() == 0x99);

    const auto &state = mockLinuxWireState();
    assert(state.readCalls == 1);
    assert(state.ioctlReadCalls == 0);

    tw.end();
}

int main()
{
    testPlainReadUsesRead();
    testRepeatedStartUsesIoctl();
    testInternalAddressClamp();
    testTimeoutFlagOnReadFailure();
    testDeferredWriteFlushes();
    testTxBufferOverflow();
    testFlushOnDifferentAddress();
    testZeroInternalAddressFallback();

    std::puts("linux_wire tests passed");
    return 0;
}
