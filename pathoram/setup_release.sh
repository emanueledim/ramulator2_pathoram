#!/bin/sh

if [ -d "build" ]; then
  rm -r build
fi
mkdir build
cd build
../../cmake-3.27.9-linux/bin/cmake ..