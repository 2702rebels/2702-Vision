cmake_minimum_required(VERSION 3.16)
project(log-visualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(log-visualizer log_visualizer.cpp)
target_link_libraries(log-visualizer ${OpenCV_LIBS} Qt6::Widgets)
target_include_directories(log-visualizer PRIVATE ${OpenCV_INCLUDE_DIRS})
