sudo apt install -y libprotobuf-dev libssh-dev libopencv-dev cmake build-essential ffmpeg nlohmann-json3-dev libcamera-dev ninja-build doxygen pkg-config git protobuf-compiler libasio-dev
echo "Downloading and compiling wpilib..."
git clone https://github.com/wpilibsuite/allwpilib.git
cd allwpilib
cmake . --preset default -DBUILD_SHARED_LIBS=OFF -DWITH_GUI=OFF -DWITH_SIMULATION_MODULES=OFF -DWITH_TESTS=OFF
cd build-cmake
sudo cmake --build . --target install --parallel $(nproc)
cd ..
cd ..
sudo rm -rf allwpilib
echo "Downloading and compiling gtest"
git clone https://github.com/google/googletest.git
cd googletest
mkdir build
cd build
cmake ..
make
sudo make install
cd ..
cd ..
sudo rm -rf googletest
echo "Downloading and compiling spdlog"
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build
cd build
cmake .. -DSPDLOG_USE_STD_FORMAT=ON -DSPDLOG_INSTALL=ON -DSPDLOG_BUILD_TESTS=OFF -DSPDLOG_BUILD_TESTS_HO=OFF -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_BUILD_EXAMPLE_HO=OFF -DSPDLOG_BUILD_BENCH=OFF
sudo cmake --build . --target install --parallel $(nproc)
cd ..
cd ..
sudo rm -rf spdlog
echo "Finished installing dependencies"