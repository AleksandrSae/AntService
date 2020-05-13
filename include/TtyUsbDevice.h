/**
 *  AntStick -- communicate with an ANT+ USB stick
 *  Copyright (C) 2017 - 2020 Alexander Saechnikov (saechnikov.a@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Defaults.h"
#include "Device.h"

#include <string>

class TtyUsbDevice: public Device {
public:
    TtyUsbDevice() : path_to_device_(DEFAULT_TTY_USB_FULL_PATH) {};
    TtyUsbDevice(std::string const & path_to_device) : path_to_device_(path_to_device) {};
    virtual void Read(std::vector<uint8_t> &) override;
    virtual void Write(std::vector<uint8_t> const &) override;
    virtual void Connect() override;
    virtual void Disconnect() override;

    typedef int FileDescriptorType;

    virtual ~TtyUsbDevice() override;

private:
    std::string path_to_device_;
    int device_baudrate_ = DEFAULT_TTY_USB_DEVICE_BAUDRATE;
    FileDescriptorType tty_usb_file_ = 0;
    struct termios tty_ {};
    bool connected_ = false;
    std::vector<uint8_t> stored_chunck_ {};
};
