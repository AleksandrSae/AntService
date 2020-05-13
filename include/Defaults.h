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

#include <termios.h> // POSIX terminal control definitions
#include <cstdint>
#include <vector>
#include <bitset>

#define DEFAULT_TTY_USB_FULL_PATH "/dev/ttyUSB0"
#define DEFAULT_TTY_USB_DEVICE_BAUDRATE B115200

namespace HRM {

    // Values taken from the HRM ANT+ Device Profile document
    enum {
        ANT_DEVICE_TYPE = 0x78,
        CHANNEL_PERIOD = 8070,
        CHANNEL_FREQUENCY = 57,
        SEARCH_TIMEOUT = 30
    };

    enum {
        STALE_TIMEOUT = 5000
    };

};

namespace ant
{
enum MessageId {
    SYNC_BYTE = 0xA4,
    INVALID = 0x00,

    // Configuration messages
    UNASSIGN_CHANNEL = 0x41,
    ASSIGN_CHANNEL = 0x42,
    SET_CHANNEL_ID = 0x51,
    SET_CHANNEL_PERIOD = 0x43,
    SET_CHANNEL_SEARCH_TIMEOUT = 0x44,
    SET_CHANNEL_RF_FREQ = 0x45,
    SET_NETWORK_KEY = 0x46,
    SET_TRANSMIT_POWER = 0x47,
    SET_SEARCH_WAVEFORM = 0x49, // XXX: Not in official docs
    ADD_CHANNEL_ID = 0x59,
    CONFIG_LIST = 0x5A,
    SET_CHANNEL_TX_POWER = 0x60,
    LOW_PRIORITY_CHANNEL_SEARCH_TIMOUT = 0x63,
    SERIAL_NUMBER_SET_CHANNEL = 0x65,
    ENABLE_EXT_RX_MESGS = 0x66,
    ENABLE_LED = 0x68,
    ENABLE_CRYSTAL = 0x6D,
    LIB_CONFIG = 0x6E,
    FREQUENCY_AGILITY = 0x70,
    PROXIMITY_SEARCH = 0x71,
    CHANNEL_SEARCH_PRIORITY = 0x75,
    // SET_USB_INFO                       = 0xff

    // Notifications
    STARTUP_MESSAGE = 0x6F,
    SERIAL_ERROR_MESSAGE = 0xAE,

    // Control messages
    RESET_SYSTEM = 0x4A,
    OPEN_CHANNEL = 0x4B,
    CLOSE_CHANNEL = 0x4C,
    OPEN_RX_SCAN_MODE = 0x5B,
    REQUEST_MESSAGE = 0x4D,
    SLEEP_MESSAGE = 0xC5,

    // Data messages
    BROADCAST_DATA = 0x4E,
    ACKNOWLEDGE_DATA = 0x4F,
    BURST_TRANSFER_DATA = 0x50,

    // Responses (from channel)
    CHANNEL_RESPONSE = 0x40,

    // Responses (from REQUEST_MESSAGE, 0x4d)
    RESPONSE_CHANNEL_STATUS = 0x52,
    RESPONSE_CHANNEL_ID = 0x51,
    RESPONSE_VERSION = 0x3E,
    RESPONSE_CAPABILITIES = 0x54,
    RESPONSE_SERIAL_NUMBER = 0x61
};

enum error_types {
    NO_ERROR = 0,
    NOT_CONNECTED,
    UNEXPECTED_MESSAGE,
    BAD_CHANNEL_RESPONSE,
    _ERROR_TYPES_COUNT
};

typedef std::bitset<_ERROR_TYPES_COUNT> error;

enum ChannelType {
    BIDIRECTIONAL_RECEIVE = 0x00,
    BIDIRECTIONAL_TRANSMIT = 0x10,

    SHARED_BIDIRECTIONAL_RECEIVE = 0x20,
    SHARED_BIDIRECTIONAL_TRANSMIT = 0x30,

    UNIDIRECTIONAL_RECEIVE_ONLY = 0x40,
    UNIDIRECTIONAL_TRANSMIT_ONLY = 0x50
};

const uint8_t Default_network = 0;

const std::vector<uint8_t> AntPlusNetworkKey {
    Default_network, 0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45 };

}
