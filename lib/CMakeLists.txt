
add_library(2702 STATIC)

file(GLOB_RECURSE 2702_SOURCES "implementation/*.cpp")
if(CMAKE_CUDA_HOST_COMPILER)
    message(STATUS "Building lib2702 for Jetson")
    target_include_directories(2702 PRIVATE /usr/local/cuda/include)
    file(GLOB_RECURSE CUDA_SOURCES "implementation/*.cudacpp")
    list(APPEND 2702_SOURCES ${CUDA_SOURCES})
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    message(STATUS "Building lib2702 for RPI")
    file(GLOB_RECURSE RPI_SOURCES "implementation/*.rpicpp")
    list(APPEND 2702_SOURCES ${RPI_SOURCES})
else()
    message(STATUS "Building lib2702 for desktop")
endif()
target_sources(2702 PRIVATE ${2702_SOURCES})

target_include_directories(2702 PUBLIC "include")

find_package(OpenCV REQUIRED)
target_link_libraries(2702 PRIVATE ${OpenCV_LIBS})
target_include_directories(2702 PRIVATE ${OpenCV_INCLUDE_DIRS})