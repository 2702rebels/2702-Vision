set -e
set -o pipefail
LIBFILE="/usr/local/lib/libgtest.a"
if [ ! -f "$LIBFILE" ]; then
    ./installpackages.sh
fi
make
test/test