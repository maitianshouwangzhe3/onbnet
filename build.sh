#!/bin/bash
if [ ! -d "build" ]; then
    mkdir build
fi

cmake -DCMAKE_BUILD_TYPE=Debug build/
make