cmake_minimum_required ( VERSION 3.4.0 )

include_directories (
  include
)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable( sample
                src/sample.cpp
)

if (UNIX)
    target_link_libraries( sample
        pthread
        AntService
    )
else (UNIX)
    target_link_libraries( sample
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/AntService.lib
    )
endif (UNIX)
