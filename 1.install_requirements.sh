#!/bin/bash

echo "Installing requirements..."

sudo wget https://github.com/Kitware/CMake/releases/download/v3.27.9/cmake-3.27.9-linux-x86_64.tar.gz
sudo tar -xzvf cmake-3.27.9-linux-x86_64.tar.gz

echo "Done."