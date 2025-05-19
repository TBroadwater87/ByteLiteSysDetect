cmake_minimum_required(VERSION 3.20)
project(ByteLiteSysDetect VERSION 1.0 LANGUAGES CXX)

add_library(ByteLiteSysDetect STATIC
add_library(ByteLiteSysDetect STATIC
    src/ByteLiteSysDetect/ByteLiteSysDetect.cpp
)

target_include_directories(ByteLiteSysDetect PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


target_compile_features(ByteLiteSysDetect PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(ByteLiteSysDetect PRIVATE /MT)
endif()
