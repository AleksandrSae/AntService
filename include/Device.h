#include "Common.h"

class Device {
public:
    virtual void Read(Buffer *buff) = 0;
    virtual void Write(const Buffer &) = 0;
    virtual void Connect() = 0; 
    virtual ~Device() {}
};
