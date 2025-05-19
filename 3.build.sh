#!/bin/bash
cd ramulator2
if [ -d "build" ]; then
  rm -r build
fi
mkdir build
cd build
../../cmake-3.27.9-linux-x86_64/bin/cmake ..
make -j4
cp ./ramulator2 ../ramulator2

echo "Done."