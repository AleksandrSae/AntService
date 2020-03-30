#include <iostream>
#include "TtyUsbDevice.h"
#include "Stick.h"

int main() {
    Stick stick = Stick();
    stick.AttachDevice(std::unique_ptr<Device>(new TtyUsbDevice()));

    stick.Connect();

    stick.Reset();

    stick.Connect();

    //Buffer test_message {'H', 'e', 'l', 'l', 'o', '\n'};
    //stick.WriteMsg(std::move(test_message));
    stick.QueryInfo();

    Buffer input = Buffer();
    stick.ReadMsg(&input);
    for (auto symbol : input) {
        std::cout << symbol;
    }
    /*
    TtyUsbDevice tty = TtyUsbDevice();

    tty.Connect();

    Buffer test_message {'H', 'e', 'l', 'l', 'o'};
    tty.Write(test_message);

    Buffer buff = Buffer();
    tty.Read(&buff);

    for (auto symbol : buff ) {
        std::cout << symbol;
    }
*/ return 0; }
