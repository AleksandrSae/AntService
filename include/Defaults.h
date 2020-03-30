#pragma once

#include <termios.h> // POSIX terminal control definitions

#define DEFAULT_TTY_USB_DEVICE_NAME "/dev/ttyUSB0"
#define DEFAULT_TTY_USB_DEVICE_BAUDRATE B115200

namespace ant {
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

enum error {
    NO_ERROR = 0,
    NOT_CONNECTED,
    UNEXPECTED_MESSAGE
};
}
