cmake_minimum_required(VERSION 3.16)
project(cli-vpn CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# header files (.hpp)
include_directories(include)

# List the source files we want to compile right now
add_executable(cli-vpn 
    src/main.cpp
)