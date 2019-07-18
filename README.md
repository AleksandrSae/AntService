# Read data

This is a service allowing to read data from ANT+ devices. It can currently
read heart rate from an ANT+ HRM.

## Dependencies

lubusb - https://github.com/libusb/libusb
gtests - https://github.com/google/googletest

## Building the application

The application is built on a Windows platform using Visual Studio 2019 (the
Community Edition will work fine) and Linux.

1. Get AntService create directory 'build' got to build (`mkdir build && cd build`)
2. Create make files by cmake:
    * Linux: `cmake ..`
    * Windows: `cmake -A x64 -G "Visual Studio 16 2019" ..`
3. Build service:
    * Linux: `make`
    * Windows:
        * `MSBuild AntService.vcxproj /property:Configuration=Release /property:Platform=x64`
        * `MSBuild tests/api_tests.vcxproj /property:Configuration=Release /property:Platform=x64` - build tests
        * `MSBuild samples/sample.vcxproj /property:Configuration=Release /property:Platform=x64` - build sample

## SetUp environment

To connet with Ant device need to install libusbK driver.
You can use Zadig (https://zadig.akeo.ie/) application: select your Ant device and libusbK driver.

## Running the application

To run the application, open a command window and type:

    ./sample.exe

The application will try to find the ANT+ USB stick and connect to the heart
rate monitor.
