#include "AntStickCommon.h"

using namespace ant_stick;

void CheckChannelResponse (
    const Buffer &response, uint8_t channel, uint8_t cmd, uint8_t status)
{
#if !defined(FAKE_CALL)
    if (response.size() < 6
        || response[2] != CHANNEL_RESPONSE
        || response[3] != channel
        || response[4] != cmd
        || response[5] != status)
    {
#if defined (DEBUG_DUMP)
        DumpData(&response[0], response.size(), std::cerr);
        std::cerr << "expecting channel: " << (int)channel
                  << ", cmd  " << (int)cmd << ", status " << (int)status << "\n";
#endif
        // Funny thing: this function is also called from a destructor while
        // an exception is being unwound.  Don't cause further trouble...
        if (! std::uncaught_exception())
            throw std::runtime_error ("CheckChannelResponse -- bad response");
    }
#endif
}

void AddMessageChecksum (Buffer &b)
{
    uint8_t c = 0;
    std::for_each (b.begin(), b.end(), [&](uint8_t e) { c ^= e; });
    b.push_back (c);
}

Buffer MakeMessage (AntMessageId id, uint8_t data)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (0x01);                 // data length
    b.push_back (static_cast<uint8_t>(id));
    b.push_back (data);
    AddMessageChecksum (b);
    return b;
}

Buffer MakeMessage (AntMessageId id, uint8_t data0, uint8_t data1)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (0x02);                 // data length
    b.push_back (static_cast<uint8_t>(id));
    b.push_back (data0);
    b.push_back (data1);
    AddMessageChecksum (b);
    return b;
}

Buffer MakeMessage (AntMessageId id,
                    uint8_t data0, uint8_t data1, uint8_t data2)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (0x03);                 // data length
    b.push_back (static_cast<uint8_t>(id));
    b.push_back (data0);
    b.push_back (data1);
    b.push_back (data2);
    AddMessageChecksum (b);
    return b;
}

Buffer MakeMessage (AntMessageId id,
                    uint8_t data0, uint8_t data1, uint8_t data2,
                    uint8_t data3, uint8_t data4)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (0x05);                 // data length
    b.push_back (static_cast<uint8_t>(id));
    b.push_back (data0);
    b.push_back (data1);
    b.push_back (data2);
    b.push_back (data3);
    b.push_back (data4);
    AddMessageChecksum (b);
    return b;
}

Buffer MakeMessage (AntMessageId id, Buffer data)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (static_cast<uint8_t>(data.size()));
    b.push_back (static_cast<uint8_t>(id));
    b.insert (b.end(), data.begin(), data.end());
    AddMessageChecksum (b);
    return b;
}

Buffer MakeMessage (AntMessageId id, uint8_t data0, const Buffer &data)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (static_cast<uint8_t>(data.size() + 1));
    b.push_back (static_cast<uint8_t>(id));
    b.push_back (data0);
    b.insert (b.end(), data.begin(), data.end());
    AddMessageChecksum (b);
    return b;
}

Buffer MakeMessage (AntMessageId id, uint8_t data0, uint8_t data1, const Buffer &data)
{
    Buffer b;
    b.push_back (SYNC_BYTE);
    b.push_back (static_cast<uint8_t>(data.size() + 2));
    b.push_back (static_cast<uint8_t>(id));
    b.push_back (data0);
    b.push_back(data1);
    b.insert (b.end(), data.begin(), data.end());
    AddMessageChecksum (b);
    return b;
}

std::ostream& operator<< (std::ostream &out, const AntChannel::Id &id)
{
    out << "#<ID Type = " << (int)id.DeviceType << "; Number = " << id.DeviceNumber << ">";
    return out;
}

};                                      // end anonymous namespace



