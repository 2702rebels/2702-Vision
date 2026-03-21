cmake_minimum_required(VERSION 3.10)
project(log-visualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc imgcodecs)

add_executable(log-visualizer log_visualizer.cpp)
target_link_libraries(log-visualizer ${OpenCV_LIBS})
target_include_directories(log-visualizer PRIVATE ${OpenCV_INCLUDE_DIRS})
