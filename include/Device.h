#pragma once

#include "Common.h"
#include <iostream>

class Device {
public:
    virtual void Read(std::vector<uint8_t> *buff) = 0;
    virtual void Write(const std::vector<uint8_t> &) = 0;
    virtual void Connect() = 0;
    virtual ~Device() {}
    Device() {}
    Device(const Device &dev)
    {
        LOG_FUNC;
        //LOG_MSG("Copy constructor worked here!");
    }
};
