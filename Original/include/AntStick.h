/**
 *  AntStick -- communicate with an ANT+ USB stick
 *  Copyright (C) 2017 - 2019 Alex Harsanyi (AlexHarsanyi@gmail.com),
 *                            Alexey Kokoshnikov (alexeikokoshnikov@gmail.com)
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
#include <queue>
#include <stdint.h>
#include <condition_variable>

// TODO: move libusb in the C++ file
#pragma warning (push)
#pragma warning (disable: 4200)
#include "libusb.h"
#pragma warning (pop)

#include "AntStickCommon.h"

class AntMessageReader;
class AntMessageWriter;
class AntStick;

const char *ChannelEventAsString(ant_stick::AntChannelEvent e);


// ......................................................... AntChannel ....

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
        Id(uint8_t device_type, uint32_t device_number = 0)
            : TransmissionType(0),
            DeviceType(device_type),
            DeviceNumber(device_number)
        {
            // empty
        }

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

    AntChannel(AntStick *stick,
        Id channel_id,
        unsigned period,
        uint8_t timeout,
        uint8_t frequency);
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


// ........................................................... AntStick ....

/** Exception thrown when the ANT stick is not found (perhaps because it is
    not plugged into a USB port). */
class AntStickNotFound : public std::exception
{
public:
    const char * what() const noexcept override;
};


/**
    * Represents the physical USB ANT Stick used to communicate with ANT+
    * devices.  An ANT Stick manages one or more AntChannel instances.  The
    * Tick() method needs to be called periodically to process the received
    * messages and distribute them to the AntChannel instances.  In addition to
    * that, `libusb_handle_events_timeout_completed` or equivalent needs to be
    * called periodically to allow libusb to process messages.  See also
    * `TickAntStick()`
    *
    * @hint Don't forget to call libusb_init() somewhere in your program before
    * using this class.
    */
class AntStick
{
    friend AntChannel;

public:
    AntStick();
    ~AntStick();

    void SetNetworkKey(uint8_t key[8]);

    unsigned GetSerialNumber() const { return m_SerialNumber; }
    std::string GetVersion() const { return m_Version; }
    int GetMaxNetworks() const { return m_MaxNetworks; }
    int GetMaxChannels() const { return m_MaxChannels; }
    int GetNetwork() const { return m_Network; }

    void WriteMessage(const ant_stick::Buffer &b);
    const ant_stick::Buffer& ReadMessage();

    void Tick();

    static uint8_t g_AntPlusNetworkKey[8];

private:

    void Reset();
    void QueryInfo();
    void RegisterChannel(AntChannel *c);
    void UnregisterChannel(AntChannel *c);
    int NextChannelId() const;

    bool MaybeProcessMessage(const ant_stick::Buffer &message);

    libusb_device *m_Device;
    libusb_device_handle *m_DeviceHandle;

    unsigned m_SerialNumber;
    std::string m_Version;
    int m_MaxNetworks;
    int m_MaxChannels;

    int m_Network;

    std::queue <ant_stick::Buffer> m_DelayedMessages;
    ant_stick::Buffer m_LastReadMessage;

    std::unique_ptr<AntMessageReader> m_Reader;
    std::unique_ptr<AntMessageWriter> m_Writer;

    std::vector<AntChannel*> m_Channels;
    std::vector<int> m_ChannelsWaitingCraetion;
};

/** Read ANT messages from an USB device (the ANT stick) */
class AntMessageReader
{
public:
    AntMessageReader(libusb_device_handle *dh, uint8_t endpoint);
#if defined (FAKE_CALL)
    AntMessageReader(uint8_t endpoint)
        :m_Endpoint(endpoint),
         m_Transfer(nullptr),
         m_Mark(0),
         m_Active(false)
    {
        m_Buffer.reserve(1024);
        m_Transfer = libusb_alloc_transfer(0);
    }
#endif
    ~AntMessageReader();

    void MaybeGetNextMessage(ant_stick::Buffer &message);
    void GetNextMessage(ant_stick::Buffer &message);

private:

    void GetNextMessage1(ant_stick::Buffer &message);

    static void LIBUSB_CALL Trampoline(libusb_transfer *);
    void SubmitUsbTransfer();
    void CompleteUsbTransfer(const libusb_transfer *);

    libusb_device_handle *m_DeviceHandle;
    uint8_t m_Endpoint;
    libusb_transfer *m_Transfer;

    /** Hold partial data received from the USB stick.  A single USB read
        * might not return an entire ANT message. */
    ant_stick::Buffer m_Buffer;
    unsigned m_Mark;            // buffer position up to where data is available
    bool m_Active;              // is there a transfer active?
};

/** Write ANT messages to a USB device (the ANT stick). */
class AntMessageWriter
{
public:
    AntMessageWriter(libusb_device_handle *dh, uint8_t endpoint);
#if defined (FAKE_CALL)
    AntMessageWriter(uint8_t endpoint)
        :m_Endpoint(endpoint),
         m_Transfer(nullptr),
         m_Active(false)
    {
        m_Transfer = libusb_alloc_transfer(0);
    }
#endif
    ~AntMessageWriter();

    void WriteMessage(const ant_stick::Buffer &message);

private:

    static void LIBUSB_CALL Trampoline(libusb_transfer *);
    void SubmitUsbTransfer();
    void CompleteUsbTransfer(const libusb_transfer *);

    libusb_device_handle *m_DeviceHandle;
    uint8_t m_Endpoint;
    libusb_transfer *m_Transfer;

    ant_stick::Buffer m_Buffer;
    bool m_Active;                      // is there a transfer active?
};

/** Call libusb_handle_events_timeout_completed() than the AntStick's Tick()
    * method.  This is an all-in-one function to get the AntStick to work, but it
    * is only appropriate if the application communicates with a single USB
    * device.
    *
    * @hint Don't forget to call libusb_init() somewhere in your program before
    * using this function.
    */
void TickAntStick(AntStick *s);

    class AntStickReader
    {
    public:
        AntStickReader();
        ~AntStickReader();
    };

/*
    Local Variables:
    mode: c++
    End:
*/
