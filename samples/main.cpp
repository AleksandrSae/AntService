#include <iostream>
#include "TtyUsbDevice.h"

int main() {
    TtyUsbDevice tty = TtyUsbDevice();
    
    tty.Connect();
    
    Buffer test_message {'H', 'e', 'l', 'l', 'o'};
    tty.Write(test_message);

    Buffer buff = Buffer();
    tty.Read(&buff);

    for (auto symbol : buff ) {
        std::cout << symbol;
    }

    return 0;
}
