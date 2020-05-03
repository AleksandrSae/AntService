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

    bool ReadExtendedMsg(uint8_t &channel_number, std::vector<uint8_t> &payload, uint16_t &device_number, uint8_t &device_type, uint8_t &trans_type) {
        LOG_FUNC;

        std::vector<uint8_t> buff {};

        m_device->Read(buff);
        if (buff.size() != 18 or buff[2] != 0x4e or buff[12] != 0x80) { LOG_ERR("This message is not extended data message"); return false; }

        channel_number = buff[3];
        for (int i=4; i<12; ++i)
            payload.push_back(buff[i]);
        //device_number = buff[14] << 8 + buff[13];
        device_number = (uint16_t)buff[14] << 8 | (uint16_t)buff[13];
        device_type = buff[15];
        trans_type = buff[16];

        return true;
    }

    void SendMsg(std::vector<uint8_t> const &buff) {
        LOG_FUNC;
        m_device->Send(buff);
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

    void Init() {
        set_network_key(ant::AntPlusNetworkKey);
        assign_channel(0, 0);
        set_channel_id(0, 0, HRM::ANT_DEVICE_TYPE);
        configure_channel(0, HRM::CHANNEL_PERIOD, HRM::SEARCH_TIMEOUT, HRM::CHANNEL_FREQUENCY);
        extended_messages(true);
        open_channel(0);
    }

    ant::error Reset();

private:
    ant::error do_command(const std::vector<uint8_t> &message,
                          std::function<ant::error (const std::vector<uint8_t>&)> check,
                          uint8_t response_msg_type);
    ant::error get_serial(int& serial);
    ant::error get_version(std::string& version);
    ant::error get_capabilities(int& max_channels, int& max_networks);
    ant::error set_network_key(const std::vector<uint8_t> & network_key);
    ant::error extended_messages(bool enabled);
    ant::error assign_channel(uint8_t channel_number, uint8_t network_key);
    ant::error set_channel_id(uint8_t channel_number, uint32_t device_number, uint8_t device_type);
    ant::error configure_channel(uint8_t channel_number, uint32_t period, uint8_t timeout, uint8_t frequency);
    ant::error open_channel(uint8_t channel_number);
    ant::error check_channel_response(const std::vector<uint8_t> &response,
                                      uint8_t channel, uint8_t cmd, uint8_t status);

private:
    std::unique_ptr<Device> m_device;
    std::string m_version;
    int m_serial = 0;
    int m_channels = 0;
    int m_networks = 0;

    uint32_t m_state;
};
