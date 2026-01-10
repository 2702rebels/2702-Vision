echo "installing build requirements from apt"
sudo apt update
sudo apt install -y git cmake meson ninja-build libboost-dev libgnutls28-dev openssl libdrm-dev libexpat1-dev libjpeg-dev libtiff5-dev python3-pip python3-yaml python3-ply python3-jinja2 libboost-program-options-dev qt6-base-dev qmake6 libepoxy-dev
echo "building rpi libcamera"
git clone https://github.com/raspberrypi/libcamera
cd libcamera
meson build
ninja -C build
sudo ninja -C build install
cd ..
rm -rf libcamera
echo "Building rpicam-apps"
git clone https://github.com/raspberrypi/rpicam-apps
cd rpicam-apps
meson build
ninja -C build
sudo ninja -C build install
cd ..
rm -rf rpicam-apps
echo "Finished installing rpi dependencies"