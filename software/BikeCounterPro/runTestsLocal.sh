#!/bin/sh

BUILD_PATH="./build"

cmake -S "./src/timerSchedule" -B $BUILD_PATH
cmake --build $BUILD_PATH
cd $BUILD_PATH && ctest --rerun-failed --output-on-failure