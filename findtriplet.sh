if [ $(uname -m) = "x86_64" ]; then
    ARCH="x64"
fi
if [ $(uname -m) = "aarch64" ]; then
    ARCH="arm64"
fi
if [ $(uname -m) = "aarch32" ]; then
    ARCH="arm32"
fi
if [ $(uname -m) = "i686" ]; then
    ARCH="x86"
fi
if [ $(uname -m) = "s390x" ]; then
    ARCH="s390x"
fi
if [ $(uname -m) = "loongarch64" ]; then
    ARCH="loongarch64"
fi
if [ $(uname -m) = "loongarch32" ]; then
    ARCH="loongarch32"
fi
if [ $(uname -m) = "mips64" ]; then
    ARCH="mips64"
fi
echo "${ARCH}-linux"