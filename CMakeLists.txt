cmake_minimum_required(VERSION 3.20)
project(ByteLiteSysDetect VERSION 1.0 LANGUAGES CXX)

add_library(ByteLiteSysDetect STATIC
    ByteLiteSysDetect.cpp
)

target_include_directories(ByteLiteSysDetect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(ByteLiteSysDetect PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(ByteLiteSysDetect PRIVATE /MT)
endif()
