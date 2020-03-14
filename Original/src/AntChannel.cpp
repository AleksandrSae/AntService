#include "AntChannel.h"

const char *ChannelEventAsString(ant_stick::AntChannelEvent e)
{
    for (int i = 0; g_ChanelEventNames[i].event != LAST_EVENT_ID; i++) {
        if (g_ChanelEventNames[i].event == e)
            return g_ChanelEventNames[i].text;
    }
    return "unknown channel event";
}

void AntChannel::InternalInit(AntStick *stick)
{
    m_ChannelNumber = stick->NextChannelId();

    if (m_ChannelNumber == -1)
        throw std::runtime_error("no more channel ids left");

    // we hard code the type to BIDIRECTIONAL_RECEIVE, using other channel
    // types would require changes to the handling code anyway.
    m_Stick->WriteMessage(
        MakeMessage(
            ASSIGN_CHANNEL, m_ChannelNumber,
            static_cast<uint8_t>(BIDIRECTIONAL_RECEIVE),
            static_cast<uint8_t>(m_Stick->GetNetwork())));
    Buffer response = m_Stick->ReadMessage();
    CheckChannelResponse(response, m_ChannelNumber, ASSIGN_CHANNEL, 0);
    LOG_PRINT("ASSIGN_CHANNEL: m_ChannelNumber = %d, NetworkKey = %d\n",
              m_ChannelNumber, static_cast<uint8_t>(m_Stick->GetNetwork()));

    m_Stick->WriteMessage(
        MakeMessage(SET_CHANNEL_ID, m_ChannelNumber,
            static_cast<uint8_t>(m_ChannelId.DeviceNumber & 0xFF),
            static_cast<uint8_t>((m_ChannelId.DeviceNumber >> 8) & 0xFF),
            m_ChannelId.DeviceType,
            // High nibble of the transmission_type is the top 4 bits
            // of the 20 bit device id.
            static_cast<uint8_t>((m_ChannelId.DeviceNumber >> 12) & 0xF0)));
    response = m_Stick->ReadMessage();
    CheckChannelResponse(response, m_ChannelNumber, SET_CHANNEL_ID, 0);
    LOG_PRINT("SET_CHANNEL_ID: m_ChannelNumber = %d, m_ChannelId.DeviceNumber = %d,
              m_ChannelId.DeviceType = %d\n", m_ChannelNumber, m_ChannelId.DeviceNumber, m_ChannelId.DeviceType);

    Configure();
    LOG_PRINT("CONFIGURE_CHANNEL: period = %d, timeout = %d, frequency = %d\n", period, timeout, frequency);

    m_Stick->WriteMessage(
        MakeMessage(OPEN_CHANNEL, m_ChannelNumber));
    response = m_Stick->ReadMessage();
    CheckChannelResponse(response, m_ChannelNumber, OPEN_CHANNEL, 0);
    LOG_PRINT("OPEN_CHANNEL: m_ChannelNumber = %d\n", m_ChannelNumber);

    m_State = CH_SEARCHING;
    m_Stick->RegisterChannel(this);
}

AntChannel::AntChannel (int channel_number,
                        AntChannel::Id channel_id,
                        unsigned period,
                        uint8_t timeout,
                        uint8_t frequency)
    : m_Stick (stick),
      m_IdReqestOutstanding (false),
      m_AckDataRequestOutstanding(false),
      m_ChannelId(channel_id),
      m_period(period),
      m_timeout(timeout),
      m_frequency(frequency)
{
    InternalInit(stick);
}

AntChannel::~AntChannel()
{
    try {
        if (m_State != CH_CLOSED) {
            ChangeState(CH_CLOSED);
            RequestClose();
            // The channel has to respond with an EVENT_CHANNEL_CLOSED channel
            // event, but we cannot process that (it would go through
            // Tick(). We wait at least for the event to be generated.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            RequestUnassign();
        }
    }
    catch (std::exception &) {
        // discard it
    }

    m_Stick->UnregisterChannel (this);
}

/** Request this channel to close.  Closing the channel involves receiving a
 * status message back, so HandleMessage() still has to be called with chanel
 * messages until IsOpen() returns false.
 */
void AntChannel::RequestClose()
{
    m_Stick->WriteMessage (MakeMessage (CLOSE_CHANNEL, m_ChannelNumber));
    Buffer response = m_Stick->ReadMessage();
    CheckChannelResponse (response, m_ChannelNumber, CLOSE_CHANNEL, 0);
}

void AntChannel::RequestUnassign()
{
    m_Stick->WriteMessage(MakeMessage(UNASSIGN_CHANNEL, m_ChannelNumber));
    Buffer response = m_Stick->ReadMessage();
    CheckChannelResponse(response, m_ChannelNumber, UNASSIGN_CHANNEL, 0);
}


void AntChannel::SendAcknowledgedData(int tag, const Buffer &message)
{
    m_AckDataQueue.push(AckDataItem(tag, message));
}

void AntChannel::RequestDataPage(uint8_t page_id, int transmit_count)
{
    const uint8_t DP_DP_REQUEST = 0x46;

    Buffer msg;
    msg.push_back(DP_DP_REQUEST);
    msg.push_back(0xFF);                // slave serial LSB
    msg.push_back(0xFF);                // slave serial MSB
    msg.push_back(0xFF);                // descriptor 1
    msg.push_back(0xFF);                // descriptor 2
    // number of times we ask the slave to transmit the data page (if it is
    // lost due to channel collisions the slave won't care)
    msg.push_back(transmit_count);
    msg.push_back(page_id);
    msg.push_back(0x01);                // command type: 0x01 request data page
    SendAcknowledgedData(page_id, msg);
}

/** Configure communication parameters for the channel
 */
void AntChannel::Configure ()
{
    m_Stick->WriteMessage (
        MakeMessage (SET_CHANNEL_PERIOD, m_ChannelNumber, m_period & 0xFF, (m_period >> 8) & 0xff));
    Buffer response = m_Stick->ReadMessage();
    CheckChannelResponse (response, m_ChannelNumber, SET_CHANNEL_PERIOD, 0);

    m_Stick->WriteMessage (
        MakeMessage (SET_CHANNEL_SEARCH_TIMEOUT, m_ChannelNumber, m_timeout));
    response = m_Stick->ReadMessage();
    CheckChannelResponse (response, m_ChannelNumber, SET_CHANNEL_SEARCH_TIMEOUT, 0);

    m_Stick->WriteMessage (
        MakeMessage (SET_CHANNEL_RF_FREQ, m_ChannelNumber, m_frequency));
    response = m_Stick->ReadMessage();
    CheckChannelResponse(response, m_ChannelNumber, SET_CHANNEL_RF_FREQ, 0);
}

/** Called by the AntStick::Tick method to process a message received on this
 * channel.  This will look for some channel events, and process them, but
 * delegate most of the messages to ProcessMessage() in the derived class.
 */
void AntChannel::HandleMessage(const uint8_t *data, int size)
{
    LOG_PRINT("HandleMessage: m_ChannelNumber = %d, type = %d\n", m_ChannelNumber, data[2]);
    if (m_State == CH_CLOSED) {
        // We should not receive messages if we are closed, maybe we should
        // throw?
#if defined DEBUG_DUMP
        std::cerr << "AntChannel::HandleMessage -- received a message while closed\n";
        DumpData(data, size, std::cerr);
#endif
        return;
    }

    switch (data[2]) {
    case CHANNEL_RESPONSE:
        OnChannelResponseMessage (data, size);
        break;
    case BROADCAST_DATA:
        if (m_State != CH_OPEN && ! m_IdReqestOutstanding)
        {
            // We received a broadcast message on this channel and we don't
            // have a master serial number, find out who is sending us
            // broadcast data
            LOG_PRINT("REQUEST_MESSAGE: SET_CHANNEL_ID for m_ChannelNumber = %d\n", m_ChannelNumber);
            m_Stick->WriteMessage (
                MakeMessage (REQUEST_MESSAGE, m_ChannelNumber, SET_CHANNEL_ID));
            m_IdReqestOutstanding = true;
        }
        MaybeSendAckData();
        OnMessageReceived(data, size);
        break;
    case RESPONSE_CHANNEL_ID:
        OnChannelIdMessage (data, size);
        break;
    default:
        OnMessageReceived (data, size);
        break;
    }
}

/** Send an ACKNOWLEDGE_DATA message, if we have one to send and there are no
 * outstanding ones.
 */
void AntChannel::MaybeSendAckData()
{
    if (! m_AckDataRequestOutstanding && ! m_AckDataQueue.empty()) {
        const AckDataItem &item = m_AckDataQueue.front();
        m_Stick->WriteMessage(MakeMessage(ACKNOWLEDGE_DATA, m_ChannelNumber, item.data));
        m_AckDataRequestOutstanding = true;
    }
}

/** Process a channel response message.
 */
void AntChannel::OnChannelResponseMessage (const uint8_t *data, int size)
{
    LOG_MSG("OnChannelResponseMessage\n");
    assert(data[2] == CHANNEL_RESPONSE);

    auto msg_id = data[4];
    auto event = static_cast<ant_stick::AntChannelEvent>(data[5]);
    // msg_id should be 1 if it is a general event and an message ID if it
    // is a response to an channel message we sent previously.  We don't
    // expect chanel responses here
    if (msg_id == 1)
    {
        if (event == EVENT_RX_SEARCH_TIMEOUT) {
            // ignore it, we are closed, but we need to wait for the closed
            // message
        }
        else if (event == EVENT_CHANNEL_CLOSED) {
            // NOTE: a search timeout will close the channel.
            if (m_State != CH_CLOSED) {
                ChangeState(CH_CLOSED);
                RequestUnassign();
            }
            return;
        }
        else if (event == EVENT_RX_FAIL_GO_TO_SEARCH) {
            m_ChannelId.DeviceNumber = 0;      // lost our device
            ChangeState(CH_SEARCHING);
        }
        else if (event == RESPONSE_NO_ERROR) {
            // we seem to be getting these from time to time, ignore them
        }
        else if (m_AckDataRequestOutstanding) {
            // We received a status for a ACKNOWLEDGE_DATA transmission
            auto tag = m_AckDataQueue.front().tag;
            m_AckDataQueue.pop();
            m_AckDataRequestOutstanding = false;
            OnAcknowledgedDataReply(tag, event);
        }
        else {
#if defined DEBUG_OUTPUT
            std::cerr << "Got unexpected channel event " << (unsigned)event << ": "
                      << ChannelEventAsString (event) << "\n";
#endif
        }
    } else {
#if defined DEBUG_OUTPUT
        std::cerr << "Unexpected reply for command " << (unsigned)msg_id
                  << ": " << (unsigned) event << " " << ChannelEventAsString (event)
                  << std::endl;
#endif
    }

    return;
}

/** Process a RESPONSE_CHANNEL_ID message.  We ask for one when we receive a
 * broadcast data and we used it to identify the master device we are paired
 * with.
 */
void AntChannel::OnChannelIdMessage (const uint8_t *data, int size)
{
    LOG_MSG("OnChannelIdMessage\n");
    assert(data[2] == RESPONSE_CHANNEL_ID);

    // we asked for this when we received the first broadcast message on
    // the channel
    if (data[3] != m_ChannelNumber) {
        throw std::runtime_error ("unexpected channel number");
    }

    auto transmission_type = static_cast<TransmissionType>(data[7] & 0x03);
    // note: high nibble of the transmission type byte represents the
    // extended 20bit device number
    uint16_t device_number = data[4] | (data[5] << 8) | ((data[7] >> 4) & 0x0F) << 16;
    uint8_t device_type = data[6];

    if (m_ChannelId.DeviceType == 0) {
        m_ChannelId.DeviceType = device_type;
    } else if (m_ChannelId.DeviceType != device_type) {
        // we seem to have paired up with a different device type than we
        // asked for...
        throw std::runtime_error ("unexpected device type");
    }

    if (m_ChannelId.DeviceNumber == 0) {
        m_ChannelId.DeviceNumber = device_number;
    } else if (m_ChannelId.DeviceNumber != device_number) {
        // we seem to have paired up with a different device than we asked
        // for...
        throw std::runtime_error ("unexpected device number");
    }

    // NOTE: fist channel id responses might not contain a message ID.
    if (m_ChannelId.DeviceNumber != 0) {
        ChangeState(CH_OPEN);
        wasChannelOpen.notify_all();
#if defined DEBUG_OUTPUT
        std::cerr << "Got a device number: " << m_ChannelId.DeviceNumber << "\n";
#endif
    }

    m_IdReqestOutstanding = false;
}

/** Change the channel state to 'new_state' and call OnStateChanged() if the
 * state has actually changed.
 */
void AntChannel::ChangeState(State new_state)
{
    if (m_State != new_state) {
        OnStateChanged(m_State, new_state);
        m_State = new_state;
    }
}

void AntChannel::OnStateChanged (State old_state, State new_state)
{
    // do nothing.  An implementation is provided so any derived classes not
    // interested in state changes can just ignore this.
}

void AntChannel::OnAcknowledgedDataReply(int tag, ant_stick::AntChannelEvent event)
{
    // do nothing.  An implementation is provided so any derived classes not
    // interested in ack data replies can just ignore this.
}

