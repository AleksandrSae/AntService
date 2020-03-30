#pragma once

#include "Defaults.h"
#include "Device.h"
#include <string>

// For testing purpose
#include <iostream>

class TtyUsbDevice: public Device {
public:
    void Read(Buffer *buff) override;
    void Write(const Buffer &) override;
    void Connect() override;
    void Disconnect();

    typedef int FileDescriptorType;

    ~TtyUsbDevice();

private:
    std::string m_device_name = DEFAULT_TTY_USB_DEVICE_NAME;
    int m_device_baudrate = DEFAULT_TTY_USB_DEVICE_BAUDRATE;
    FileDescriptorType m_tty_usb_file = 0;
    struct termios m_tty = {0};
    bool m_connected = false;
};
