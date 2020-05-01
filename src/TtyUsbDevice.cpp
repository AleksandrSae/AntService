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

    if (m_connected) return;

    m_tty_usb_file = open(m_device_name.c_str(), O_RDWR);

    // Clean termios struct, we call it 'tty' for convention
    m_tty = {0};

    // Read in existing settings, and handle any error
    if(tcgetattr(m_tty_usb_file, &m_tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return;
    }

    m_tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    m_tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    m_tty.c_cflag |= CS8; // 8 bits per byte (most common)
    m_tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    m_tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    m_tty.c_lflag &= ~ICANON;
    m_tty.c_lflag &= ~ECHO; // Disable echo
    m_tty.c_lflag &= ~ECHOE; // Disable erasure
    m_tty.c_lflag &= ~ECHONL; // Disable new-line echo
    m_tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    m_tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    m_tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    m_tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    m_tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    m_tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    m_tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&m_tty, DEFAULT_TTY_USB_DEVICE_BAUDRATE);
    cfsetospeed(&m_tty, DEFAULT_TTY_USB_DEVICE_BAUDRATE);

    // Save tty settings, also checking for error
    if (tcsetattr(m_tty_usb_file, TCSANOW, &m_tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }
    m_connected = true;
}

void TtyUsbDevice::Disconnect() {
    if (m_connected) {
        close(m_tty_usb_file);
        m_connected = false;
    }
}

void TtyUsbDevice::Write(const std::vector<uint8_t> &buff) {
    if (m_connected)
        write(m_tty_usb_file, &buff[0], buff.size());
}

void TtyUsbDevice::Read(std::vector<uint8_t> &buff) {
    if (!m_connected) return;

    char read_buf[256] = {0};

    uint32_t num_bytes = 0;
    uint32_t total_bytes = 0;

    do {
        num_bytes = read(m_tty_usb_file, &read_buf, sizeof(read_buf));

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

bool TtyUsbDevice::ReadNextMessage(std::vector<uint8_t> &message) {
    LOG_FUNC;

    // Try to find SYNC_BYTE
    do {
        auto itt = std::find(stored_chunck_.begin(), stored_chunck_.end(), ant::SYNC_BYTE);
        if (itt != stored_chunck_.end()) {
            stored_chunck_.erase(stored_chunck_.begin(), itt);
            break;
        }
        Read(stored_chunck_);
    } while(true);

    // If message size <4 get new portion
    while (stored_chunck_.size() < 4)
        Read(stored_chunck_);

    unsigned int len = (unsigned int)stored_chunck_[1] + 4; // Total lenght is SYNC + LEN + MSGID + DATA  
    while (stored_chunck_.size() < len)
        Read(stored_chunck_);

    message = std::vector<uint8_t>(stored_chunck_.begin(), stored_chunck_.begin() + len);
    stored_chunck_.erase(stored_chunck_.begin(), stored_chunck_.begin() + len);

    return true;
}
