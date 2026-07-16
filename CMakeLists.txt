cmake_minimum_required(VERSION 3.16)
project(cli-vpn CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(_WIN32_WINNT=0x0A00)

# header files (.hpp)
include_directories(include)

include_directories(${CMAKE_SOURCE_DIR})

# List the source files we want to compile right now
add_executable(cli-vpn 
    src/main.cpp
    src/system/windows/process.cpp
    src/system/windows/router.cpp
    src/api/vpngate_client.cpp
    src/api/ping_helper.cpp
)

target_link_libraries(cli-vpn ws2_32 crypt32 iphlpapi)