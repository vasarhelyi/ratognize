#!/bin/bash

echo ""
echo "Configuring ratognize for host platform..."
mkdir -p build
cd build
rm -f CMakeCache.txt
cmake ..
cd ..
echo ""

