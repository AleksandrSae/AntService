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


struct ExtendedMessage {
    uint8_t channel_number;
    uint8_t payload[8];
    uint16_t device_number;
    uint8_t device_type;
    uint8_t trans_type;
};


class Stick {
public:

    void AttachDevice(std::unique_ptr<Device> && device);
    void Connect();
    bool Reset();
    bool Init();
    bool ReadNextMessage(std::vector<uint8_t> &);
    bool ReadExtendedMsg(ExtendedMessage &);

private:
    ant::error do_command(const std::vector<uint8_t> &message,
                          std::function<ant::error (const std::vector<uint8_t>&)> process,
                          uint8_t wait_response_messege_type);
    ant::error reset();
    ant::error query_info();
    ant::error get_serial(unsigned &serial);
    ant::error get_version(std::string &version);
    ant::error get_capabilities(unsigned &max_channels, unsigned &max_networks);
    ant::error set_network_key(std::vector<uint8_t> const &network_key);
    ant::error set_extended_messages(bool enabled);
    ant::error assign_channel(uint8_t channel_number, uint8_t network_key);
    ant::error set_channel_id(uint8_t channel_number, uint32_t device_number, uint8_t device_type);
    ant::error configure_channel(uint8_t channel_number, uint32_t period, uint8_t timeout, uint8_t frequency);
    ant::error open_channel(uint8_t channel_number);
    ant::error check_channel_response(const std::vector<uint8_t> &response,
                                      uint8_t channel, uint8_t cmd, uint8_t status);

private:
    std::unique_ptr<Device> device_;
    std::vector<uint8_t> stored_chunk_ {};
    std::string version_;
    unsigned serial_ = 0;
    unsigned channels_ = 0;
    unsigned networks_ = 0;
};
