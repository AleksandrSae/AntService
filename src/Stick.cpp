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

#include "Stick.h"


void Stick::AttachDevice(std::unique_ptr<Device> && device)
{
    LOG_FUNC;

    device_ = std::move(device);
}


bool Stick::Connect()
{
    LOG_FUNC;

    device_->Connect();

    return device_->IsConnected();
}


bool Stick::Reset() {
    return reset() == ant::NO_ERROR ? true : false;
}


bool Stick::Init()
{
    LOG_FUNC;

    ant::error result = ant::NO_ERROR;

    result |= query_info();
    result |= set_network_key(ant::AntPlusNetworkKey);
    // For code simplification, we set some defaults with zero values
    // TODO: Add default values to Defaults.h
    result |= assign_channel(0/*network*/, 0/*channel*/);
    result |= set_channel_id(0/*channel*/, 0/*device*/, HRM::ANT_DEVICE_TYPE);
    result |= configure_channel(0/*channel*/, HRM::CHANNEL_PERIOD, HRM::SEARCH_TIMEOUT, HRM::CHANNEL_FREQUENCY);
    result |= set_extended_messages(true);
    result |= open_channel(0);

    if (result != ant::NO_ERROR)
        return false;

    return true;
}


bool Stick::ReadNextMessage(std::vector<uint8_t> &message)
{
    LOG_FUNC;

    // Try to find SYNC_BYTE
    do {
        auto itt = std::find(stored_chunk_.begin(), stored_chunk_.end(), ant::SYNC_BYTE);
        if (itt != stored_chunk_.end()) {
            stored_chunk_.erase(stored_chunk_.begin(), itt);
            break;
        }
        device_->Read(stored_chunk_);
    } while(true);

    // If message size <4 get new portion
    while (stored_chunk_.size() < 4)
        device_->Read(stored_chunk_);

    unsigned int len = (unsigned int)stored_chunk_[1] + 4; // Total lenght is SYNC + LEN + MSGID + DATA
    while (stored_chunk_.size() < len)
        device_->Read(stored_chunk_);

    message = std::vector<uint8_t>(stored_chunk_.begin(), stored_chunk_.begin() + len);
    stored_chunk_.erase(stored_chunk_.begin(), stored_chunk_.begin() + len);

    return true;
}


bool Stick::ReadExtendedMsg(ExtendedMessage& ext_msg)
{

    /* Flagged Extended Data Message Format
     *
     * | 1B   | 1B     | 1B  | 1B      | 8B      | 1B   | 2B     | 1B     | 1B    | 1B    |
     * |------|--------|-----|---------|---------|------|--------|--------|-------|-------|
     * | SYNC | Msg    | Msg | Channel | Payload | Flag | Device | Device | Trans | Check |
     * |      | Length | ID  | Number  |         | Byte | Number | Type   | Type  | sum   |
     * |      |        |     |         |         |      |        |        |       |       |
     * | 0    | 1      | 2   | 3       | 4-11    | 12   | 13,14  | 15     | 16    | 17    |
     */

    LOG_FUNC;

    std::vector<uint8_t> buff {};

    device_->Read(buff);
    if (buff.size() != 18 or buff[2] != 0x4e or buff[12] != 0x80) {
        LOG_ERR("This message is not extended data message");
        return false;
    }

    ext_msg.channel_number = buff[3];

    for (int j=0; j<8; j++) {
        ext_msg.payload[j] = buff[j+4];
    };

    ext_msg.device_number = (uint16_t)buff[14] << 8 | (uint16_t)buff[13];
    ext_msg.device_type = buff[15];
    ext_msg.trans_type = buff[16];

    return true;
}


ant::error Stick::do_command(const std::vector<uint8_t> &message,
                             std::function<ant::error (const std::vector<uint8_t>&)> check_func,
                             uint8_t response_msg_type)
{
    LOG_FUNC;

    LOG_MSG("Write: " << MessageDump(message));
    device_->Write(std::move(message));

    std::vector<uint8_t> response_msg {};
    do {
        ReadNextMessage(response_msg);
    } while (response_msg[2] != response_msg_type);

    LOG_MSG("Read: " << MessageDump(response_msg));

    ant::error status = check_func(response_msg);
    if (status != ant::NO_ERROR)
        return status;

    return ant::NO_ERROR;
}


ant::error Stick::reset()
{
    LOG_FUNC;

    return this->do_command(Message(ant::RESET_SYSTEM, {0}),
           [] (const std::vector<uint8_t>& buff) -> ant::error {
               if (buff.size() < 2) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               return ant::NO_ERROR;
           },
           ant::STARTUP_MESSAGE);
}


ant::error Stick::query_info()
{
    LOG_FUNC;

    ant::error result = ant::NO_ERROR;

    result |= get_serial(serial_);
    LOG_MSG("Serial: " << serial_);

    result |= get_version(version_);
    LOG_MSG("Version: " << version_);

    result |= get_capabilities(channels_, networks_);
    LOG_MSG("Channels: " << channels_ << " NetWorks: " << networks_);

    return result;
}


ant::error Stick::get_serial(unsigned &serial)
{
    LOG_FUNC;

    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_SERIAL_NUMBER}),
           [&serial] (std::vector<uint8_t> const &buff) -> ant::error {
               serial = buff[3] | (buff[4] << 8) | (buff[5] << 16) | (buff[6] << 24);
               return ant::NO_ERROR;
           },
           ant::RESPONSE_SERIAL_NUMBER);
}


ant::error Stick::get_version(std::string& version)
{
    LOG_FUNC;

    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_VERSION}),
           [&version] (std::vector<uint8_t> const &buff) -> ant::error {
           // TODO: Append a string length check by getting the message length from message field
               version += reinterpret_cast<const char *>(&buff[3]);
               return ant::NO_ERROR;
           },
           ant::RESPONSE_VERSION);
}


ant::error Stick::get_capabilities(unsigned &max_channels, unsigned &max_networks)
{
    LOG_FUNC;

    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_CAPABILITIES}),
           [&max_channels, &max_networks] (std::vector<uint8_t> const &buff) -> ant::error {
               max_channels = (unsigned)buff[3];
               max_networks = (unsigned)buff[4];
               return ant::NO_ERROR;
           },
           ant::RESPONSE_CAPABILITIES);
}


ant::error Stick::check_channel_response(const std::vector<uint8_t> &response, uint8_t channel, uint8_t cmd, uint8_t status)
{
    LOG_FUNC;

    if (response.size() < 6
        || response[2] != ant::CHANNEL_RESPONSE
        || response[3] != channel
        || response[4] != cmd
        || response[5] != status)
    {
        if (! std::uncaught_exception()) {
            LOG_ERR("check_channel_response - bad response");
            return ant::BAD_CHANNEL_RESPONSE;
        }
    }

    return ant::NO_ERROR;
}


ant::error Stick::set_network_key(const std::vector<uint8_t> & network_key)
{
    LOG_FUNC;

    return this->do_command(Message(ant::SET_NETWORK_KEY, std::move(network_key)),
           [&] (const std::vector<uint8_t>& buff) -> ant::error {
               return this->check_channel_response(buff, network_key[0], ant::SET_NETWORK_KEY, 0);
           },
           ant::CHANNEL_RESPONSE);
}


ant::error Stick::set_extended_messages(bool enable = false)
{
    LOG_FUNC;

    return this->do_command({Message(ant::ENABLE_EXT_RX_MESGS, {0, static_cast<uint8_t>(enable ? 1 : 0)})},
                [] (const std::vector<uint8_t>& buff) -> ant::error {
                    return ant::NO_ERROR;
              },
              ant::CHANNEL_RESPONSE);
}


ant::error Stick::assign_channel(uint8_t channel_number, uint8_t network_number)
{
    LOG_FUNC;

    ant::error result = this->do_command(Message(ant::ASSIGN_CHANNEL, {
                channel_number, ant::BIDIRECTIONAL_RECEIVE, network_number}),
           [this, channel_number] (const std::vector<uint8_t>& buff) -> ant::error {
               return this->check_channel_response(buff, channel_number, ant::ASSIGN_CHANNEL, 0);
           },
           ant::CHANNEL_RESPONSE);

    if (result == ant::NO_ERROR)
        LOG_MSG("Assign channel number: " << std::dec << (unsigned)channel_number << " network: " << std::dec << (unsigned)network_number); 

    return result;
}


ant::error Stick::set_channel_id(uint8_t channel_number, uint32_t device_number, uint8_t device_type)
{
    LOG_FUNC;

    ant::error result = this->do_command(Message(ant::SET_CHANNEL_ID, {
                                         channel_number,
                                         static_cast<uint8_t>(device_number & 0xFF),
                                         static_cast<uint8_t>((device_number >> 8) & 0xFF),
                                         device_type,
                                         // High nibble of the transmission_type is the top 4 bits
                                         // of the 20 bit device id.
                                         static_cast<uint8_t>((device_number >> 12) & 0xF0)
                                         }),
           [this, channel_number] (const std::vector<uint8_t>& buff) -> ant::error {
               return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_ID, 0);
           },
           ant::CHANNEL_RESPONSE);

    if (result == ant::NO_ERROR)
        LOG_MSG("Set channel id: channel number: " << std::hex << (unsigned)channel_number
                << " device number: " << std::hex << (unsigned)device_number
                << " device type: " << std::hex << (unsigned)device_type);

    return result;
}


ant::error Stick::configure_channel(uint8_t channel_number, uint32_t period, uint8_t timeout, uint8_t frequency)
{
    LOG_FUNC;

    ant::error result = ant::NO_ERROR;

    result |= this->do_command({Message(ant::SET_CHANNEL_PERIOD, {
                    channel_number, static_cast<uint8_t>(period & 0xff), static_cast<uint8_t>(period >> 8 & 0xff)
                })},
                [this, channel_number] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_PERIOD, 0);
              },
              ant::CHANNEL_RESPONSE);

    result |= this->do_command({Message(ant::SET_CHANNEL_SEARCH_TIMEOUT, {channel_number, timeout})},
                [this, channel_number] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_SEARCH_TIMEOUT, 0);
              },
              ant::CHANNEL_RESPONSE);

    result |= this->do_command({Message(ant::SET_CHANNEL_RF_FREQ, {channel_number, frequency})},
                [this, channel_number] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_RF_FREQ, 0);
              },
              ant::CHANNEL_RESPONSE);

    return result;
}


ant::error Stick::open_channel(uint8_t channel_number)
{
    LOG_FUNC;

    return this->do_command({Message(ant::OPEN_CHANNEL, {channel_number})},
                [this, channel_number] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::OPEN_CHANNEL, 0);
              },
              ant::CHANNEL_RESPONSE);
}
