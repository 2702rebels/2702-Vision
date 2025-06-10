sudo apt install -y curl autoconf autoconf-archive python3 python3.12-venv automake libtool pkg-config libltdl-dev libudev-dev libx11-dev libxft-dev libxext-dev libxi-dev libxtst-dev libxrandr-dev git build-essential bzip2 '^libxcb.*-dev' libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev
sudo apt-get update
sudo apt-get install -y bison nasm flex
# Will switch back to microsoft master when latest wpilib is merged there
git clone https://github.com/OhmV-IR/vcpkg.git
cd vcpkg
git checkout fmt-update
cd ..
vcpkg/bootstrap-vcpkg.sh # downloads vcpkg binary
if [ $(./findtriplet.sh) = "x64-linux" ]; then
    echo "set(VCPKG_BUILD_TYPE release)" >> vcpkg/triplets/x64-linux.cmake
else 
    echo "set(VCPKG_BUILD_TYPE release)" >> vcpkg/triplets/community/$(./findtriplet.sh).cmake
fi
vcpkg/vcpkg install --clean-after-build
echo $(pwd)/vcpkg_installed/$(./findtriplet.sh)/lib | sudo tee -a /etc/ld.so.conf.d/vcpkglibs.conf
sudo ldconfig
sudo ln -s $(pwd)/vcpkg_installed/$(./findtriplet.sh)/share/ffmpeg /usr/local/bin/ffmpeg