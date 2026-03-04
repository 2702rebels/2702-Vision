#!/bin/bash
git clone https://github.com/2702rebles/apriltags_cuda
(cd apriltags_cuda; cmake -B build -DCMAKE_CUDA_COMPILER=clang++-17 -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_CUDA_ARCHITECTURES=87)
(cd apriltags_cuda; make --build build)
git clone https://github.com/devernay/cminpack.git
