
add_library(2702 STATIC)

file(GLOB_RECURSE 2702_SOURCES "implementation/*.cpp")
message(STATUS ${2702_SOURCES})
if(CMAKE_CUDA_HOST_COMPILER)
    message(STATUS "Building for Jetson")
    target_include_directories(2702 PRIVATE /usr/local/cuda/include)
    file(GLOB_RECURSE CUDA_SOURCES "implementation/*.cudacpp")
    list(APPEND 2702_SOURCES ${CUDA_SOURCES})
endif()
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    message(STATUS "Building for Raspberry Pi platform")
    file(GLOB_RECURSE RPI_SOURCES "implementation/*.rpicpp")
    list(APPEND 2702_SOURCES ${RPI_SOURCES})
endif()
message(STATUS ${2702_SOURCES})
target_sources(2702 PRIVATE ${2702_SOURCES})

target_include_directories(2702 PUBLIC "include")

find_package(OpenCV REQUIRED)
target_link_libraries(2702 PRIVATE ${OpenCV_LIBS})
target_include_directories(2702 PRIVATE ${OpenCV_INCLUDE_DIRS})