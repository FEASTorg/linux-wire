#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct MockLinuxWireState
{
    int openCalls = 0;
    int closeCalls = 0;
    std::string lastDevicePath;
    int setSlaveCalls = 0;
    uint8_t lastSetSlaveAddr = 0;
    int writeCalls = 0;
    std::vector<uint8_t> lastWriteBuffer;
    bool lastWriteWasIoctl = false;
    uint8_t lastWriteSlaveAddr = 0;
    int logErrors = 1;
    int readCalls = 0;
    std::vector<uint8_t> lastReadBuffer;
    int ioctlReadCalls = 0;
    uint16_t lastIoctlAddr = 0;
    std::vector<uint8_t> lastIoctlInternal;
};

void mockLinuxWireReset();
void mockLinuxWireSetReadData(const std::vector<uint8_t> &data);
void mockLinuxWireSetIoctlReadData(const std::vector<uint8_t> &data);
void mockLinuxWireForceReadError(int err);
void mockLinuxWireClearReadError();
const MockLinuxWireState &mockLinuxWireState();
