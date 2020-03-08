#pragma once

#define TIMEOUT 2000

namespace ant_stick {

typedef std::vector<uint8_t> Buffer;

enum AntMessageId {
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

/** Channel events received as part of the CHANNEL_RESPONSE message, defined
    * in section 9.5.6 "Channel Response / Event Messages" in
    * D00000652_ANT_Message_Protocol_and_Usage_Rev_5.1
    */
enum AntChannelEvent {
    RESPONSE_NO_ERROR = 0,
    EVENT_RX_SEARCH_TIMEOUT = 1,
    EVENT_RX_FAIL = 2,
    EVENT_TX = 3,
    EVENT_TRANSFER_RX_FAILED = 4,
    EVENT_TRANSFER_TX_COMPLETED = 5,
    EVENT_TRANSFER_TX_FAILED = 6,
    EVENT_CHANNEL_CLOSED = 7,
    EVENT_RX_FAIL_GO_TO_SEARCH = 8,
    EVENT_CHANNEL_COLLISION = 9,
    EVENT_TRANSFER_TX_START = 10,
    EVENT_TRANSFER_NEXT_DATA_BLOCK = 17,
    CHANNEL_IN_WRONG_STATE = 21,
    CHANNEL_NOT_OPENED = 22,
    CHANNEL_ID_NOT_SET = 24,
    CLOSE_ALL_CHANNELS = 25,
    TRANSFER_IN_PROGRESS = 31,
    TRANSFER_SEQUENCE_NUMBER_ERROR = 32,
    TRANSFER_IN_ERROR = 33,
    MESSAGE_SIZE_EXCEEDS_LIMIT = 39,
    INVALID_MESSAGE = 40,
    INVALID_NETWORK_NUMBER = 41,
    INVALID_LIST_ID = 48,
    INVALID_SCAN_TX_CHANNEL = 49,
    INVALID_PARAMETER_PROVIDED = 51,
    EVENT_SERIAL_QUE_OVERFLOW = 52,
    EVENT_QUE_OVERFLOW = 53,
    ENCRYPT_NEGOTIATION_SUCCESS = 56,
    ENCRYPT_NEGOTIATION_FAIL = 57,
    NVM_FULL_ERROR = 64,
    NVM_WRITE_ERROR = 65,
    USB_STRING_WRITE_FAIL = 112,
    MESG_SERIAL_ERROR_ID = 174,
    LAST_EVENT_ID = 0xff
};

enum TransmissionType {
    ANT_INDEPENDENT_CHANNEL = 0x01
};

constexpr int MaxTries = 50;

enum ChannelType {
    BIDIRECTIONAL_RECEIVE = 0x00,
    BIDIRECTIONAL_TRANSMIT = 0x10,

    SHARED_BIDIRECTIONAL_RECEIVE = 0x20,
    SHARED_BIDIRECTIONAL_TRANSMIT = 0x30,

    UNIDIRECTIONAL_RECEIVE_ONLY = 0x40,
    UNIDIRECTIONAL_TRANSMIT_ONLY = 0x50
};

struct ChannelEventName {
    AntChannelEvent event;
    const char *text;
} static g_ChanelEventNames[] = {
    { RESPONSE_NO_ERROR, "no error" },
    { EVENT_RX_SEARCH_TIMEOUT, "channel search timeout" },
    { EVENT_RX_FAIL, "rx fail" },
    { EVENT_TX, "broadcast tx complete" },
    { EVENT_TRANSFER_RX_FAILED, "rx transfer fail" },
    { EVENT_TRANSFER_TX_COMPLETED, "tx complete" },
    { EVENT_TRANSFER_TX_FAILED, "tx fail" },
    { EVENT_CHANNEL_CLOSED, "channel closed" },
    { EVENT_RX_FAIL_GO_TO_SEARCH, "dropped to search mode" },
    { EVENT_CHANNEL_COLLISION, "channel collision" },
    { EVENT_TRANSFER_TX_START, "burst transfer start" },
    { EVENT_TRANSFER_NEXT_DATA_BLOCK, "burst next data block" },
    { CHANNEL_IN_WRONG_STATE, "channel in wrong state" },
    { CHANNEL_NOT_OPENED, "channel not opened" },
    { CHANNEL_ID_NOT_SET, "channel id not set" },
    { CLOSE_ALL_CHANNELS, "all channels closed" },
    { TRANSFER_IN_PROGRESS, "transfer in progress" },
    { TRANSFER_SEQUENCE_NUMBER_ERROR, "transfer sequence error" },
    { TRANSFER_IN_ERROR, "burst transfer error" },
    { MESSAGE_SIZE_EXCEEDS_LIMIT, "message too big" },
    { INVALID_MESSAGE, "invalid message" },
    { INVALID_NETWORK_NUMBER, "invalid network number" },
    { INVALID_LIST_ID, "invalid list id" },
    { INVALID_SCAN_TX_CHANNEL, "attempt to transmit in ANT channel 0 in scan mode" },
    { INVALID_PARAMETER_PROVIDED, "invalid parameter" },
    { EVENT_SERIAL_QUE_OVERFLOW, "output serial overflow" },
    { EVENT_QUE_OVERFLOW, "input serial overflow" },
    { ENCRYPT_NEGOTIATION_SUCCESS, "encrypt negotiation success" },
    { ENCRYPT_NEGOTIATION_FAIL, "encrypt negotiation fail" },
    { NVM_FULL_ERROR, "nvm full" },
    { NVM_WRITE_ERROR, "nvm write fail" },
    { USB_STRING_WRITE_FAIL, "usb write fail" },
    { MESG_SERIAL_ERROR_ID, "bad usb packet received" },
    { LAST_EVENT_ID, nullptr}};

void AddMessageChecksum (Buffer &b);
void CheckChannelResponse(const Buffer &response, uint8_t channel, uint8_t cmd, uint8_t status);
Buffer MakeMessage (AntMessageId id, uint8_t data);
Buffer MakeMessage (AntMessageId id, uint8_t data0, uint8_t data1);
Buffer MakeMessage (AntMessageId id, uint8_t data0, uint8_t data1, uint8_t data2);
Buffer MakeMessage (AntMessageId id, uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4);
Buffer MakeMessage (AntMessageId id, Buffer data);
Buffer MakeMessage (AntMessageId id, uint8_t data0, const Buffer &data);
Buffer MakeMessage (AntMessageId id, uint8_t data0, uint8_t data1, const Buffer &data);


};
