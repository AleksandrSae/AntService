/**
 *  AntStick -- communicate with an ANT+ USB stick
 *  Copyright (C) 2017 - 2020 Alex Harsanyi (AlexHarsanyi@gmail.com),
 *                            Alexey Kokoshnikov (alexeikokoshnikov@gmail.com)
 *                            Alexander Saechnikov (saechnikov.a@gmail.com)
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

    bool ReadExtendedMsg(uint8_t &channel_number,
                         std::vector<uint8_t> &payload,
                         uint16_t &device_number,
                         uint8_t &device_type,
                         uint8_t &trans_type)
    {
        LOG_FUNC;

        std::vector<uint8_t> buff {};

        m_device->Read(buff);
        if (buff.size() != 18 or buff[2] != 0x4e or buff[12] != 0x80) { LOG_ERR("This message is not extended data message"); return false; }

        channel_number = buff[3];
        for (int i=4; i<12; ++i)
            payload.push_back(buff[i]);
        device_number = (uint16_t)buff[14] << 8 | (uint16_t)buff[13];
        device_type = buff[15];
        trans_type = buff[16];

        return true;
    }

    void SendMsg(std::vector<uint8_t> const &buff) {
        LOG_FUNC;
        m_device->Write(buff);
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
        LOG_FUNC;

        ant::error result = ant::NO_ERROR;

        result |= set_network_key(ant::AntPlusNetworkKey);
        result |= assign_channel(0, 0);
        result |= set_channel_id(0, 0, HRM::ANT_DEVICE_TYPE);
        result |= configure_channel(0, HRM::CHANNEL_PERIOD, HRM::SEARCH_TIMEOUT, HRM::CHANNEL_FREQUENCY);
        result |= extended_messages(true);
        result |= open_channel(0);
    }

    ant::error Reset();

private:
    ant::error do_command(const std::vector<uint8_t> &message,
                          std::function<ant::error (const std::vector<uint8_t>&)> check,
                          uint8_t response_msg_type);
    ant::error get_serial(int &serial);
    ant::error get_version(std::string &version);
    ant::error get_capabilities(unsigned &max_channels, unsigned &max_networks);
    ant::error set_network_key(std::vector<uint8_t> const &network_key);
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
    unsigned m_channels = 0;
    unsigned m_networks = 0;

    uint32_t m_state;
};
