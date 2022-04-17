cmake_minimum_required(VERSION 3.16)

add_library(rtclib RTClib_mock.cpp RTClib_mock.h)

target_include_directories(rtclib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")