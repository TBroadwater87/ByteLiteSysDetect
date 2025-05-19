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

target_compile_features(ByteLiteSysDetect PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(ByteLiteSysDetect PRIVATE /MT)
endif()

option(BYTELITE_SYSDETECT_BUILD_TESTS "Build ByteLiteSysDetect tests" ON)

if(BYTELITE_SYSDETECT_BUILD_TESTS)
    enable_testing()
    add_executable(ByteLiteSysDetectTest tests/test_main.cpp)
    target_link_libraries(ByteLiteSysDetectTest PRIVATE ByteLiteSysDetect)
    add_test(NAME ByteLiteSysDetectTest COMMAND ByteLiteSysDetectTest)
endif()
