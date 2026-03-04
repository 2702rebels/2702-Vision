git clone https://github.com/2702rebels/apriltags_cuda --recursive
cd apriltags_cuda
cmake -B build -DCMAKE_CUDA_COMPILER=clang++-17 -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_CUDA_ARCHITECTURES=87
cmake --build build
