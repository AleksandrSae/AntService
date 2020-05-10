#include <iostream>
#include "TtyUsbDevice.h"
#include "Stick.h"

int main() {
    Stick stick = Stick();
    stick.AttachDevice(std::unique_ptr<Device>(new TtyUsbDevice("/dev/ttyUSB0")));

    stick.Connect();
    stick.Reset();
    stick.Init();

    for (int i=0; i<50; i++) {

        ExtendedMessage msg;

        if (stick.ReadExtendedMsg(msg)) {
            std::cout << "Channel:" << std::dec << (unsigned) msg.channel_number
                      << " Payload:";

            for (int j = 0; j < 8; ++j)
                std::cout << " 0x" << std::hex << (unsigned) msg.payload[j];

            std::cout << " Device number:" << std::dec << (unsigned) msg.device_number
                      << " Device type:0x" << std::hex << (unsigned) msg.device_type
                      << " Transfer type:0x" << std::hex << (unsigned) msg.trans_type
                      << std::endl;
        }
    }

    return 0;
}
