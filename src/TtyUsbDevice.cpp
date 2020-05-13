/**
*  MIT License
*
*  Copyright (c) 2019 Geoffrey Benjamin Mark Hunter
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#include "TtyUsbDevice.h"

#include <iostream>

// Linux headers
#include <fcntl.h>   // Contains file controls like O_RDWR
#include <errno.h>   // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()

#include <stdio.h>
#include <string.h>

TtyUsbDevice::~TtyUsbDevice() {
    LOG_FUNC;

    this->Disconnect();
}

void TtyUsbDevice::Connect() {
    LOG_FUNC;

    if (connected_) return;

    tty_usb_file_ = open(path_to_device_.c_str(), O_RDWR);

    // Clean termios struct, we call it 'tty' for convention
    tty_ = {0};

    // Read in existing settings, and handle any error
    if(tcgetattr(tty_usb_file_, &tty_) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return;
    }

    tty_.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty_.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty_.c_cflag |= CS8; // 8 bits per byte (most common)
    tty_.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty_.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty_.c_lflag &= ~ICANON;
    tty_.c_lflag &= ~ECHO; // Disable echo
    tty_.c_lflag &= ~ECHOE; // Disable erasure
    tty_.c_lflag &= ~ECHONL; // Disable new-line echo
    tty_.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty_.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty_.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty_.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty_.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty_.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty_.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty_, DEFAULT_TTY_USB_DEVICE_BAUDRATE);
    cfsetospeed(&tty_, DEFAULT_TTY_USB_DEVICE_BAUDRATE);

    // Save tty settings, also checking for error
    if (tcsetattr(tty_usb_file_, TCSANOW, &tty_) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }
    connected_ = true;
}

void TtyUsbDevice::Disconnect() {
    if (connected_) {
        close(tty_usb_file_);
        connected_ = false;
    }
}

void TtyUsbDevice::Write(const std::vector<uint8_t> &buff) {
    if (connected_) {
        int bytes = write(tty_usb_file_, &buff[0], buff.size());
        if (bytes < 0) std::cerr << "Error writing: " << errno << " : " << strerror(errno) << std::endl;
    }
}

void TtyUsbDevice::Read(std::vector<uint8_t> &buff) {
    if (!connected_) return;

    char read_buf[256] = {0};

    uint32_t num_bytes = 0;
    uint32_t total_bytes = 0;

    do {
        num_bytes = read(tty_usb_file_, &read_buf, sizeof(read_buf));

        if (num_bytes < 0) {
            std::cerr << "Error reading: " << errno << " : " << strerror(errno) << std::endl;
            return;
        }

        for (unsigned int i = 0; i < num_bytes; i++) {
            buff.push_back(read_buf[i]);
        }

        total_bytes += num_bytes;
    } while (total_bytes <= 4);
}
