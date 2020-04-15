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
