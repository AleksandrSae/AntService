#pragma once

#include <memory>
#include <functional>

#include "Defaults.h"
#include "Device.h"

class Stick {
public:

    enum State {
        DISCONNECTED = 0,
        DETACHED,
        CONNECTED,
        ERROR = 255
    };

    Stick() : m_state(DETACHED) {}

    void AttachDevice(std::unique_ptr<Device> && device) {
        LOG_FUNC;

        m_device = std::move(device);
        m_state = DISCONNECTED;
    }

    void Connect() {
        LOG_FUNC;
        m_device->Connect();
        m_state = CONNECTED;
    }

    void WriteMsg(Buffer&& buff) {
        LOG_FUNC;
        m_device->Write(buff);
    }

    void ReadMsg(Buffer* buff) {
        LOG_FUNC;
        m_device->Read(buff);
    }

    void QueryInfo() {
        LOG_FUNC;

        get_serial(m_serial);
        LOG_MSG("Serial: " << m_serial); 

        get_version(m_version);
        LOG_MSG("Version: " << m_version); 

        get_capabilities(m_channels, m_networks);
        LOG_MSG("Channels: " << m_channels << " NetWorks: " << m_networks);
    }

    ant::error Reset();

private:
    ant::error get_serial(int& serial);
    ant::error get_version(std::string& version);
    ant::error get_capabilities(int& max_channels, int& max_networks);
    ant::error do_command(const Buffer &message, std::function<ant::error (const Buffer&)> check);

private:
    std::unique_ptr<Device> m_device;
    std::string m_version;
    int m_serial = 0;
    int m_channels = 0;
    int m_networks = 0;

    uint32_t m_state;
};
