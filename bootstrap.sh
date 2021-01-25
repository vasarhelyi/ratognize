#!/bin/bash

echo ""
echo "Configuring ratognize for host platform..."
mkdir -p build
cd build
rm -f CMakeCache.txt
cmake ..
make clean
cd ..
echo ""

