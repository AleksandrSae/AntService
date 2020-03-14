cmake_minimum_required ( VERSION 3.4.0 )

if (UNIX)
    set ( CONFIGURE_CMD cmake . -Dgtest_disable_pthreads=ON )
    set ( BUILD_CMD make )
else (UNIX)
    set ( CONFIGURE_CMD "" )
    set ( BUILD_CMD MSBuild ${CMAKE_BINARY_DIR}/thirdparty/gtests/googletest/gtest.sln /property:Configuration=Release /property:Platform=x64 )
endif (UNIX)

ExternalProject_Add ( gtests
    PREFIX gtests
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR ${CMAKE_BINARY_DIR}/thirdparty/gtests
    CONFIGURE_COMMAND ${CONFIGURE_CMD}
    BUILD_COMMAND ${BUILD_CMD}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
    LOG_BUILD 1
)

include_directories (
  include
  ${CMAKE_BINARY_DIR}/thirdparty/gtests/googletest/include
)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable( api_tests
                src/api_tests.cpp
)

add_dependencies ( api_tests gtests )
if (UNIX)
    target_link_libraries(api_tests
        ${CMAKE_BINARY_DIR}/thirdparty/gtests/lib/libgtest.a
        ${CMAKE_BINARY_DIR}/thirdparty/gtests/lib/libgtest_main.a
        AntService
    )
else (UNIX)
    target_link_libraries(api_tests
        ${CMAKE_BINARY_DIR}/thirdparty/gtests/lib/Release/gtest.lib
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/AntService.lib
    )
endif (UNIX)
