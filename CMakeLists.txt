cmake_minimum_required ( VERSION 3.4.0 )

project ( AntService CXX )

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )

include_directories (
  include
  ${LIBUSB_INCLUDE_DIR}
)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

set ( SOURCE_LIB
        src/Stick.cpp
        src/TtyUsbDevice.cpp
)

add_library( AntService SHARED ${SOURCE_LIB} )

add_subdirectory( samples )
