#pragma once

#include "Defaults.h"
#include "Device.h"
#include <string>

class TtyUsbDevice: public Device {
public:
    virtual void Read(std::vector<uint8_t> &) override;
    virtual bool ReadNextMessage(std::vector<uint8_t> &) override;
    virtual void Write(const std::vector<uint8_t> &) override;
    virtual void Connect() override;
    virtual void Disconnect() override;

    typedef int FileDescriptorType;

    virtual ~TtyUsbDevice() override;

private:
    std::string m_device_name = DEFAULT_TTY_USB_DEVICE_NAME;
    int m_device_baudrate = DEFAULT_TTY_USB_DEVICE_BAUDRATE;
    FileDescriptorType m_tty_usb_file = 0;
    struct termios m_tty = {0};
    bool m_connected = false;
    std::vector<uint8_t> stored_chunck_ {0};
};
