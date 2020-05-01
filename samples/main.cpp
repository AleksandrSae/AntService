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

    std::vector<uint8_t> input;

    for (int i=0; i<500; i++) {
        stick.ReadMsg(input);
        std::cout << MessageDump(input);
        input.clear();
    }

    return 0;
}
