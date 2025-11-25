
add_library(2702 STATIC)

file(GLOB_RECURSE 2702_SOURCES "implementation/*")
if(CMAKE_CUDA_HOST_COMPILER)
    message(STATUS "Building for Jetson")
    target_include_directories(myapp PRIVATE /usr/local/cuda/include)
    target_sources(myapp PRIVATE nvcam.o)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    message(STATUS "Building for Desktop")
    list(REMOVE_ITEM 2702_SOURCES "implementation/nvcam.cpp")
    list(REMOVE_ITEM 2702_SOURCES "implementation/rpicam.cpp")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    message(STATUS "Building for Raspberry Pi platform")
    list(REMOVE_ITEM 2702_SOURCES "implementation/nvcam.cpp")
else()
    message(FATAL_ERROR "Unknown platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()
target_sources(2702 PRIVATE ${2702_SOURCES})

target_include_directories(2702 PUBLIC "include")

find_package(OpenCV REQUIRED)
target_link_libraries(2702 PRIVATE ${OpenCV_LIBS})
target_include_directories(2702 PRIVATE ${OpenCV_INCLUDE_DIRS})