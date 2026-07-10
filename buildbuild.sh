#!/bin/bash
export CC=/opt/homebrew/bin/gcc-13
export CXX=/opt/homebrew/bin/g++-13
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
export CFLAGS="-isysroot $SDKROOT"
export CXXFLAGS="-isysroot $SDKROOT"
export LDFLAGS="-isysroot $SDKROOT"

mkdir -p build
cd build
cmake ..
cmake --build .