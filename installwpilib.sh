#!/bin/bash
root=${PWD}
echo ${PWD}
git clone https://github.com/wpilibsuite/allwpilib.git
cd ${PWD}/allwpilib
mkdir build-cmake
cmake --preset default -DWITH_GUI=OFF
cd build-cmake
cmake --build . --parallel 4
sudo cmake --build . --target install
cd /etc/ld.so.conf.d
sudo touch wpilib.conf
sudo echo "/usr/local/lib/" >> wpilib.conf
cd $root/2025/Object
sudo ldconfig