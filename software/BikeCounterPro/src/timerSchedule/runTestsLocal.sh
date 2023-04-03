#!/bin/sh

BUILD_PATH="../../build/unittests/timerSchedule"

cmake -S . -B $BUILD_PATH
cmake --build $BUILD_PATH
cd $BUILD_PATH && ctest --rerun-failed --output-on-failure