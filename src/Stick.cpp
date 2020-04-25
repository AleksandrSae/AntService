#include "Stick.h"

ant::error Stick::get_serial(int& serial) {
    LOG_FUNC;
    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_SERIAL_NUMBER}),
           [&serial] (const std::vector<uint8_t>& buff) -> ant::error {
               if (buff.size() < 7 || buff[2] != ant::RESPONSE_SERIAL_NUMBER) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               serial = buff[3] | (buff[4] << 8) | (buff[5] << 16) | (buff[6] << 24);
               return ant::NO_ERROR;
           });
}


ant::error Stick::get_version(std::string& version) {
    LOG_FUNC;
    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_VERSION}),
           [&version] (const std::vector<uint8_t>& buff) -> ant::error {
               if (buff.size() < 4 || buff[2] != ant::RESPONSE_VERSION) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               version += reinterpret_cast<const char *>(&buff[3]);
               return ant::NO_ERROR;
           });
}


ant::error Stick::get_capabilities(int& max_channels, int& max_networks) {
    LOG_FUNC;
    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_CAPABILITIES}),
           [&max_channels, &max_networks] (const std::vector<uint8_t>& buff) -> ant::error {
               if (buff.size() < 2 || buff[2] != ant::RESPONSE_CAPABILITIES) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               max_channels = (int)buff[3];
               max_networks = (int)buff[4];
               return ant::NO_ERROR;
           });
}

ant::error Stick::set_network_key(const std::vector<uint8_t> & network_key) {
    LOG_FUNC;

    return this->do_command(Message(ant::SET_NETWORK_KEY, std::move(network_key)),
           [&] (const std::vector<uint8_t>& buff) -> ant::error {
               return this->check_channel_response(buff, network_key[0], ant::SET_NETWORK_KEY, 0);
           });
}

ant::error Stick::Reset() {
    LOG_FUNC;

    return this->do_command(Message(ant::RESET_SYSTEM, {0}),
           [] (const std::vector<uint8_t>& buff) -> ant::error {
               if (buff.size() < 2 || buff[2] != ant::STARTUP_MESSAGE) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               return ant::NO_ERROR;
           });
}


ant::error Stick::do_command(const std::vector<uint8_t> &message, std::function<ant::error (const std::vector<uint8_t>&)> check) {
    LOG_FUNC;

    if (m_state != CONNECTED) {
        LOG_ERR("device isn't connected");
        return ant::NOT_CONNECTED;
    }

    LOG_MSG("Write: " << MessageDump(message));
    m_device->Write(std::move(message));

    std::vector<uint8_t> response_msg;
    m_device->Read(&response_msg);
    LOG_MSG("Read: " << MessageDump(response_msg));

    ant::error status = check(response_msg);
    if (status != ant::NO_ERROR)
        return status;

    return ant::NO_ERROR;
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


ant::error Stick::assign_channel(uint8_t channel_number, uint8_t network_number)
{
    LOG_FUNC;

    // we hard code the type to BIDIRECTIONAL_RECEIVE, using other channel
    // types would require changes to the handling code anyway.
    ant::error result = this->do_command(Message(ant::ASSIGN_CHANNEL, {
                channel_number, ant::BIDIRECTIONAL_RECEIVE, network_number}),
           [&] (const std::vector<uint8_t>& buff) -> ant::error {
               return this->check_channel_response(buff, channel_number, ant::ASSIGN_CHANNEL, 0);
           });

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
           [&] (const std::vector<uint8_t>& buff) -> ant::error {
               return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_ID, 0);
           });

    if (result == ant::NO_ERROR)
        LOG_MSG("Set channel id: channel number: " << std::hex << (unsigned)channel_number
                << " device number: " << std::hex << (unsigned)device_number
                << " device type: " << std::hex << (unsigned)device_type);

    return result;
}


ant::error Stick::configure_channel(uint8_t channel_number, uint32_t period, uint8_t timeout, uint8_t frequency) {
    LOG_FUNC;

    ant::error result {ant::NO_ERROR};

    result |= this->do_command({Message(ant::SET_CHANNEL_PERIOD, {
                    channel_number, static_cast<uint8_t>(period & 0xff), static_cast<uint8_t>(period >> 8 & 0xff)
                })},
                [&] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_PERIOD, 0);
              });

    result |= this->do_command({Message(ant::SET_CHANNEL_SEARCH_TIMEOUT, {channel_number, timeout})},
                [&] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_SEARCH_TIMEOUT, 0);
              });

    result |= this->do_command({Message(ant::SET_CHANNEL_RF_FREQ, {channel_number, frequency})},
                [&] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::SET_CHANNEL_RF_FREQ, 0);
              });

    return result;
}


ant::error Stick::open_channel(uint8_t channel_number) {
    LOG_FUNC;

    return this->do_command({Message(ant::OPEN_CHANNEL, {channel_number})},
                [&] (const std::vector<uint8_t>& buff) -> ant::error {
                    return this->check_channel_response(buff, channel_number, ant::OPEN_CHANNEL, 0);
              });
}
