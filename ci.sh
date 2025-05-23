set -e
set -o pipefail
if [ -d $(pwd)/vcpkg_installed ]; then
    echo "deps installed"
else
    ./installpackages.sh
fi
cd lib
make
cd ..
cd 2025/Object
if command -v rpicam-vid &> /dev/null; then
    make
else
    make norpicam
fi
cd ..
cd ..
cd tests
make
./test