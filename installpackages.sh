sudo apt update
sudo apt install -y libprotobuf-dev libssh-dev libopencv-dev build-essential ffmpeg libavdevice-dev libavfilter-dev nlohmann-json3-dev libcamera-dev ninja-build doxygen pkg-config git protobuf-compiler libasio-dev protobuf-compiler
echo "Downloading latest CMake"
sudo apt remove --purge --auto-remove cmake -y
sudo apt install -y ca-certificates gpg wget
test -f /usr/share/doc/kitware-archive-keyring/copyright ||
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt update
test -f /usr/share/doc/kitware-archive-keyring/copyright ||
sudo rm /usr/share/keyrings/kitware-archive-keyring.gpg
sudo apt install kitware-archive-keyring -y
sudo apt update
sudo apt install cmake -y
echo "Downloading and compiling wpilib..."
git clone https://github.com/wpilibsuite/allwpilib.git
cd allwpilib
git checkout 1efaaefd78a634599b522fc2420c7283722554fb #v2025.3.2
cmake . --preset default -DBUILD_SHARED_LIBS=OFF -DWITH_GUI=OFF -DWITH_SIMULATION_MODULES=OFF -DWITH_TESTS=OFF
cd build-cmake
sudo cmake --build . --target install --parallel $(nproc)
# remove add_dependency(wpilibj) which is still added even when wpilib wasn't built with wpilibj included.
sudo sed -i '/find_dependency(wpilibj)/d' /usr/local/share/wpilib/wpilib-config.cmake
cd ..
cd ..
sudo rm -rf allwpilib
echo "Downloading and compiling gtest"
git clone https://github.com/google/googletest.git
cd googletest
git checkout 52eb8108c5bdec04579160ae17225d66034bd723 #v1.17.0
mkdir build
cd build
cmake ..
sudo cmake --build . --target install --parallel $(nproc)
cd ..
cd ..
sudo rm -rf googletest
echo "Downloading and compiling spdlog"
git clone https://github.com/gabime/spdlog.git
cd spdlog
git checkout 486b55554f11c9cccc913e11a87085b2a91f706f #v1.16.0
mkdir build
cd build
cmake .. -DSPDLOG_INSTALL=ON -DSPDLOG_BUILD_TESTS=OFF -DSPDLOG_BUILD_TESTS_HO=OFF -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_BUILD_EXAMPLE_HO=OFF -DSPDLOG_BUILD_BENCH=OFF
sudo cmake --build . --target install --parallel $(nproc)
cd ..
cd ..
sudo rm -rf spdlog
echo "Finished installing dependencies"
