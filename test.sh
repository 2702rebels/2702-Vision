set -e
set -o pipefail
LIBFILE="/usr/local/lib/libgtest.a"
if [ ! -f "$LIBFILE" ]; then
    sudo ./installpackages.sh
fi
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
tests/tests