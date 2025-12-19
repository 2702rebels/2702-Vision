add_library(2702 STATIC)

file(GLOB 2702_SOURCES "implementation/*.cpp")

if(IS_JETSON)
    target_include_directories(2702 PRIVATE /usr/local/cuda/include)
    file(GLOB CUDA_SOURCES "implementation/cuda/*.cpp")
    list(APPEND 2702_SOURCES ${CUDA_SOURCES})
endif()

if(IS_RPI)
    file(GLOB RPI_SOURCES "implementation/rpi/*.cpp")
    list(APPEND 2702_SOURCES ${RPI_SOURCES})
    add_package_to_target("libcamera" "2702" FALSE)
endif()

target_sources(2702 PRIVATE ${2702_SOURCES})

target_include_directories(2702 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_package_to_target("opencv" "2702" FALSE)
add_package_to_target("spdlog" "2702" TRUE)