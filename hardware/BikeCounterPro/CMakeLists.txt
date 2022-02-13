cmake_minimum_required(VERSION 3.18)
project("unittestproject")

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
FetchContent_Declare(
  rtclib
  GIT_REPOSITORY https://github.com/adafruit/RTClib.git
  GIT_TAG master
)

FetchContent_Declare(
  adafruitbusio
  GIT_REPOSITORY https://github.com/adafruit/Adafruit_BusIO.git
  GIT_TAG master
  SOURCE_SUBDIR nonexistingfolder
)
# nonexistingfolder due to CMakeLists.txt file in the repo

FetchContent_Declare(
  arduinocore
  GIT_REPOSITORY https://github.com/arduino/ArduinoCore-samd.git
  GIT_TAG master
)

FetchContent_Declare(
  arduinocoreapi
  GIT_REPOSITORY https://github.com/arduino/ArduinoCore-API.git
  GIT_TAG master
)

FetchContent_Declare(
  arduinoatmel
  GIT_REPOSITORY https://github.com/arduino/ArduinoModule-CMSIS-Atmel.git
  GIT_TAG master
)


# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest rtclib adafruitbusio arduinocore arduinocoreapi arduinoatmel)

add_library(rtclib STATIC ${rtclib_SOURCE_DIR}/src)
add_library(adafruitbusio STATIC ${adafruitbusio_SOURCE_DIR})
add_library(arduinocore STATIC ${arduinocore_SOURCE_DIR}/cores/arduino)
add_library(wire STATIC ${arduinocore_SOURCE_DIR}/libraries/wire)
add_library(arduinocorevar STATIC ${arduinocore_SOURCE_DIR}/variants/mkrwan1300)
add_library(arduinocoreapi STATIC ${arduinocoreapi_SOURCE_DIR})
add_library(arduinoatmel STATIC ${arduinoatmel_SOURCE_DIR}/cmsis-atmel/cmsis/device/atmel)
set_target_properties(rtclib PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(adafruitbusio PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(arduinocore PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(wire PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(arduinocorevar PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(arduinocoreapi PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(arduinoatmel PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(rtclib PUBLIC ${rtclib_SOURCE_DIR}/src)
target_include_directories(adafruitbusio PUBLIC ${adafruitbusio_SOURCE_DIR})
target_include_directories(arduinocore PUBLIC ${arduinocore_SOURCE_DIR}/cores/arduino)
target_include_directories(wire PUBLIC ${arduinocore_SOURCE_DIR}/libraries/wire)
target_include_directories(arduinocorevar PUBLIC ${arduinocore_SOURCE_DIR}/variants/mkrwan1300)
target_include_directories(arduinocoreapi PUBLIC ${arduinocoreapi_SOURCE_DIR})
target_include_directories(arduinoatmel PUBLIC ${arduinoatmel_SOURCE_DIR}/cmsis-atmel/cmsis/device/atmel)

enable_testing()

add_executable(unittest unitTests.cc)
target_link_libraries(unittest gtest_main rtclib adafruitbusio arduinocore arduinocoreapi arduinoatmel arduinocorevar wire)


include(GoogleTest)
gtest_discover_tests(unittest)
