#include "Stick.h"

ant::error Stick::get_serial(int& serial) {
    LOG_FUNC;
    return this->do_command(Message(ant::REQUEST_MESSAGE, {0, ant::RESPONSE_SERIAL_NUMBER}),
           [&serial] (const Buffer& buff) -> ant::error {
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
           [&version] (const Buffer& buff) -> ant::error {
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
           [&max_channels, &max_networks] (const Buffer& buff) -> ant::error {
               if (buff.size() < 2 || buff[2] != ant::RESPONSE_CAPABILITIES) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               max_channels = (int)buff[3];
               max_networks = (int)buff[4];
               return ant::NO_ERROR;
           });
}


ant::error Stick::Reset() {
    LOG_FUNC;

    return this->do_command(Message(ant::RESET_SYSTEM, {0}),
           [] (const Buffer& buff) -> ant::error {
               if (buff.size() < 2 || buff[2] != ant::STARTUP_MESSAGE) {
                   LOG_ERR("unexpected message");
                   return ant::UNEXPECTED_MESSAGE;
               }
               return ant::NO_ERROR;
           });
}


ant::error Stick::do_command(const Buffer &message, std::function<ant::error (const Buffer&)> check) {
    LOG_FUNC;

    if (m_state != CONNECTED) {
        LOG_ERR("device isn't connected");
        return ant::NOT_CONNECTED;
    }

    LOG_MSG("Write: " << MessageDump(message));
    m_device->Write(std::move(message));

    Buffer response_msg;
    m_device->Read(&response_msg);
    LOG_MSG("Read: " << MessageDump(response_msg));

    ant::error status = check(response_msg);
    if (status != ant::NO_ERROR)
        return status;

    return ant::NO_ERROR;
}
