
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
endif()

target_sources(2702 PRIVATE ${2702_SOURCES})

target_include_directories(2702 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(OpenCV REQUIRED)
target_link_libraries(2702 PUBLIC ${OpenCV_LIBS})
target_include_directories(2702 PUBLIC ${OpenCV_INCLUDE_DIRS})