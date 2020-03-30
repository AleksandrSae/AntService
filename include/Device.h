#pragma once

#include "Common.h"
#include <iostream>

class Device {
public:
    virtual void Read(Buffer *buff) = 0;
    virtual void Write(const Buffer &) = 0;
    virtual void Connect() = 0;
    virtual ~Device() {}
    Device() {}
    Device(const Device &dev)
    {
        LOG_MSG("Copy constructor worked here!");
    }
};
