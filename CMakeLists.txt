cmake_minimum_required(VERSION 4.2)
project(2702Vision)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CUDA_HOST_COMPILER)
    message(STATUS "Building for Jetson")
    set(IS_JETSON TRUE)
    add_compile_definitions(JETSON=1)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    message(STATUS "Building for RPI")
    set(IS_RPI TRUE)
    add_compile_definitions(RPI=1)
else()
    message(STATUS "Building for desktop")
    set(IS_DESKTOP TRUE)
    add_compile_definitions(DESKTOP=1)
endif()

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif ()
if(MSVC)
    add_compile_options(/utf-8)
endif ()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/Od)
    else ()
        add_compile_options(-O0)
    endif ()
endif ()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/Ox /fp:fast /GL /Ob3)
        add_link_options(/LTCG)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
            add_compile_options(/favor:AMD64)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
            add_compile_options(/favor:INTEL64)
        endif ()
    else ()
        add_compile_options(-Ofast)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            add_compile_options(-march=native)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            add_compile_options(-march=armv8-a)
        endif()
    endif ()
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/MultiSourcePackageFinder.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PackageFinders.cmake")

add_subdirectory(lib) # ** Important: must be first to define library targets prior to initialization of other targets

# ─────────────────────────────────────────────────────────────────────────────
# BUILD TARGET SELECTION
# ─────────────────────────────────────────────────────────────────────────────
# This project runs on three different machines:
#
#   1. Desktop (your laptop or PC running Linux)
#        → Used for development and testing tools like calib-select/sensor-calib
#        → Does NOT have wpilib (the FRC robot library) installed
#        → IS_DESKTOP = TRUE when CMake doesn't detect CUDA or aarch64
#
#   2. Jetson Orin (the vision computer on the robot)
#        → Runs the main apriltag/vision pipeline at full speed using the GPU
#        → Has CUDA and wpilib installed
#        → IS_JETSON = TRUE when a CUDA host compiler is detected
#
#   3. Raspberry Pi
#        → Alternative vision computer (no GPU)
#        → IS_RPI = TRUE when running on aarch64 without CUDA
#
# Some subdirectories (2025/Object, calibrationdevice) depend on wpilib,
# which connects to the robot's NetworkTables system. wpilib is complex to
# install on a desktop and you don't need it just to work on vision tools.
#
# So: when building on a desktop, we SKIP those subdirectories entirely.
# The Utilities folder (where calib-select and sensor-calib live) always builds
# on all platforms — it only needs OpenCV, which is easy to install.
#
# If you're getting a "wpilib not found" error and you're just trying to
# build the calibration tools, you're probably building on desktop and
# something above went wrong with IS_DESKTOP detection. Check that you
# are NOT passing -DCMAKE_CUDA_HOST_COMPILER and are on an x86_64 machine.
# ─────────────────────────────────────────────────────────────────────────────

if(NOT IS_DESKTOP)
    # The main object detection binary — needs wpilib to talk to the robot.
    # Skip this on desktop since wpilib is not installed there.
    add_subdirectory(2025/Object)
endif()

add_subdirectory(Utilities)  # Always builds — calib-select and sensor-calib live here

if(NOT IS_DESKTOP)
    # The calibration device firmware controller — also needs wpilib.
    # Skip on desktop for the same reason as 2025/Object above.
    add_subdirectory(calibrationdevice)
endif()

add_subdirectory(tests)
