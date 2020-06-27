# Read data

This is a prototype of application allowing to read data from ANT+ devices based on AntService project https://github.com/akokoshn/AntService. It can currently read heart rate from an ANT+ HRM.

## Dependencies
Linux, g++, python3, distutils

## Building the application

The application can be built on the Linux only.

1. Get AntService, create directory 'build' (`mkdir build && cd build`)
2. Create make files by cmake:
    `cmake ..`
3. Build sample application:
    `make`
4. Go to folder `python_hrm`, run build command:
    `python3 setup.py build`
5. Install the builded module
    `sudo python3 setup.py install`

## SetUp environment
Setup python3 developer package.

## Running the application
Connect ANT+ USB Transiver, check that USB device is connected:
    `dmesg`

To run the sample application, open a command window and type:

    ./sample.exe

The application will try to find the ANT+ USB stick and connect to the heart
rate monitor.

To test python module, open the command line inteface window and type:
    python3 test_application.py
