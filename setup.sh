#!/bin/bash
# Build qemu for mini2440
# make clean
sudo apt-get install zlib1g libglib2.0-dev libsdl-dev
./configure --target-list=arm-softmmu --prefix=$HOME/bin
make | tee build.info
echo "================================================"
echo "Build Success!!!"
