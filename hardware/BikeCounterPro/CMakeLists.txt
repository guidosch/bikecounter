cmake_minimum_required(VERSION 3.16)
project("unittestproject")

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 11)



include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(${PROJECT_SOURCE_DIR})
add_library(rtclib STATIC RTClib_mock.cpp RTClib_mock.h)
add_library(timerSchedule STATIC timerSchedule.cpp timerSchedule.hpp)

add_compile_definitions(UNITTEST)

add_executable(unittest unitTests.cc)
target_link_libraries(unittest rtclib timerSchedule gtest_main)


include(GoogleTest)
gtest_discover_tests(unittest)
