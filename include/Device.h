#pragma once

#include "Common.h"

class Device {
public:
    virtual void Read(std::vector<uint8_t> &) = 0;
    virtual bool ReadNextMessage(std::vector<uint8_t> &) = 0;
    virtual void Write(const std::vector<uint8_t> &) = 0;
    virtual void Connect() = 0;
    virtual void Disconnect() = 0;
    virtual ~Device() {}
};
