#include <iostream>
#include "TtyUsbDevice.h"
#include "Stick.h"

int main() {
    Stick stick = Stick();
    stick.AttachDevice(std::unique_ptr<Device>(new TtyUsbDevice()));

    stick.Connect();
    stick.Reset();
    stick.Connect();
    stick.QueryInfo();
    stick.Init();

    for (int i=0; i<500; i++) {

        uint8_t channel_number;
        std::vector<uint8_t> payload;
        uint16_t device_number;
        uint8_t device_type;
        uint8_t trans_type;

        if (stick.ReadExtendedMsg(channel_number, payload, device_number, device_type, trans_type)) {

        std::cout << "Channel:" << std::dec << (unsigned) channel_number
                  << " Payload:\"" << MessageDump(payload)
                  << "\" Device number:" << std::dec << (unsigned) device_number
                  << " Device type:0x" << std::hex << (unsigned) device_type
                  << " Transfer type:0x" << std::hex << (unsigned) trans_type
                  << std::endl;
        }

    }

    return 0;
}
