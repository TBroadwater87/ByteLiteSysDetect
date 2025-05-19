cmake_minimum_required(VERSION 3.15)
project(ByteLiteSysDetect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ByteLiteSysDetect STATIC
    src/ByteLiteSysDetect/ByteLiteSysDetect.cpp
)

target_include_directories(ByteLiteSysDetect PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(ByteLiteSysDetect PRIVATE /MT)
endif()

