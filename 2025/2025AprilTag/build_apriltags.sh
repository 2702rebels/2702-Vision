#!/bin/bash

if [ ! -d apriltags_cuda ]; then
	git clone https://2702rebels/apriltags_cuda
fi

cd apriltags_cuda
if [ ! -f deps_installed ]; then
	./install_deps.sh
	touch deps_installed
fi
cmake -B build -DCMAKE_BINARY_DIR= -DCMAKE_CUDA_COMPILER=clang++-17 -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_CUDA_ARCHITECTURES=87 -DNUM_PROCESSORS=6
cmake --build build
