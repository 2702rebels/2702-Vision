set -e
set -o pipefail
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