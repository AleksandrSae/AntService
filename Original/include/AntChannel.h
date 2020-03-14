/**
    * Represents an ANT communication channel managed by the AntStick class.
    * This class represents the "slave" endpoint, the master being the
    * device/sensor that sends the data.
    *
    * This class is not useful directly.  It needs to be derived from and at
    * least the ProcessMessage() function implemented to handle messages received
    * on the channel.  The idea is that Heart Rate, Power Meter, etc channels are
    * implemented by deriving from this class and providing access to the
    * relevant user data.
    */
class AntChannel
{
public:

    friend class AntStick;

    /** The Channel ID identifies the master we want to pair up with. In ANT+
        * terminology, a master is the actual sensor sending data, like a Heart
        * Rate monitor, and we are always the "slave".
        */
    struct Id {
        Id(uint8_t device_type, uint32_t device_number = 0) :
           TransmissionType(0),
           DeviceType(device_type),
           DeviceNumber(device_number) {}

        /** Defines the transmission type.  We always set it to 0, once paired
            * up, the master will tell us what the transmission type is.
            */
        uint8_t TransmissionType;

        /** Type of device we want to pair up with (e.g. Heart Rate Monitor,
            * Power Meter, etc).  These are IDs defines in the relevant device
            * profile in the ANT+ documentation.
            */
        uint8_t DeviceType;

        /** The serial number of the device we want to pair up with.  A value
            * of 0 indicates a "search" for any device of DeviceType type.
            */
        uint32_t DeviceNumber;
    };

    /** The state of the channel, you can get the current state with
        * ChannelState()
        */
    enum State {
        CH_SEARCHING,     // Searching for a master
        CH_OPEN,          // Open, receiving broadcast messages from a master
        CH_CLOSED         // Closed, will not receive any messages, object
                            // needs to be destroyed
    };

    AntChannel(AntStick *stick, Id channel_id, unsigned period, uint8_t timeout, uint8_t frequency);

    virtual ~AntChannel();

    void RequestClose();
    void RequestUnassign();
    State ChannelState() const { return m_State; }
    Id ChannelId() const { return m_ChannelId; }
    std::condition_variable wasChannelOpen;

protected:
    /* Derived classes can use these methods. */

    /** Send 'message' as an acknowledged message.  The actual message will
        * not be sent immediately (they can only be sent shortly after a
        * broadcast message is received).  OnAcknowledgedDataReply() will be
        * called with 'tag' and the result of the transmission.  If the
        * transmission fails, it will not be retried.
        */
    void SendAcknowledgedData(int tag, const ant_stick::Buffer &message);

    /** Ask a master device to transmit data page identified by 'page_id'.
        * The master will only send some data pages are only sent when requested
        * explicitly.  The request is sent as an acknowledged data message, but a
        * successful transmission does not mean that the master device will send
        * the data page.  The master will send these data pages as normal
        * broadcast data messages and should be processed in OnMessageReceived().
        * They will be send by the master 'transmit_count' number of times (in
        * case the data page is lost due to collisions)
        */
    void RequestDataPage(uint8_t page_id, int transmit_count = 4);

private:
    void InternalInit(AntStick *stick);
    /* Derived classes will need to override these methods */

    /** Called when a message received on this channel and it is not a status
        * message.  This should be overridden to process and interpret broadcast
        * messages received by the channel.
        */
    virtual void OnMessageReceived(const uint8_t *data, int size) = 0;

    /** Called when the state of the channel has changed. Default
        * implementation does nothing.
        */
    virtual void OnStateChanged(State old_state, State new_state);

    /** Called when we receive the status reply for an acknowledged message we
        * that was sent.  'tag' is the same tag that was passed to
        * SendAcknowledgedData() and can be used to identify which message was
        * sent (or failed to send) 'event' is one of EVENT_TRANSFER_TX_COMPLETED,
        * or EVENT_TRANSFER_TX_FAILED.  Note that failed messages are not
        * retried, the derived class can try again by calling
        * SendAcknowledgedData()
        */
    virtual void OnAcknowledgedDataReply(int tag, ant_stick::AntChannelEvent event);

private:

    State m_State;
    Id m_ChannelId;
    /** The channel number is a unique identifier assigned by the AntStick it
        * is used when assembling messages or decoding messages received by the
        * ANT Stick. */
    int m_ChannelNumber;

    /** A queued ACKNOWLEDGE_DATA message.  We can only send these messages
        * one-by-one when a broadcast message is received, so
        * SendAcknowledgedData() queues them up.
        */
    struct AckDataItem {
        AckDataItem(int t, const ant_stick::Buffer &d)
            : tag(t), data(d) {}
        int tag;
        ant_stick::Buffer data;
    };

    /** Queue of ACKNOWLEDGE_DATA messages waiting to be sent.
        */
    std::queue<AckDataItem> m_AckDataQueue;

    /** When true, an ACKNOWLEDGE_DATA message was send out and we have not
        * received confirmation for it yet.
        */
    bool m_AckDataRequestOutstanding;

    /** When true, a Channel ID request is outstanding.  We always identify
        * channels when we receive the first broadcast message on them.
        */
    bool m_IdReqestOutstanding;

    AntStick *m_Stick;
    unsigned m_period;
    uint8_t m_timeout;
    uint8_t m_frequency;

    void Configure();
    void HandleMessage(const uint8_t *data, int size);
    void MaybeSendAckData();
    void OnChannelResponseMessage(const uint8_t *data, int size);
    void OnChannelIdMessage(const uint8_t *data, int size);
    void ChangeState(State new_state);
};


