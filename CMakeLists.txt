cmake_minimum_required(VERSION 4.2)
project(2702Vision)

if(CMAKE_CUDA_HOST_COMPILER)
    message(STATUS "Building for Jetson")
    set(IS_JETSON TRUE)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    message(STATUS "Building for RPI")
    set(IS_RPI TRUE)
else()
    message(STATUS "Building for desktop")
    set(IS_DESKTOP TRUE)
endif()

add_subdirectory(lib) # ** Important: must be first to define library targets prior to initialization of other targets
add_subdirectory(2025/Object)
add_subdirectory(Utilities)
add_subdirectory(tests)