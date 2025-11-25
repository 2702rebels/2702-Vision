find_package(OpenCV REQUIRED)
find_package(cameraserver CONFIG REQUIRED)
find_package(ntcore CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)

add_executable(logffmpeg logffmpeg/logffmpeg.cpp)

target_include_directories(logffmpeg PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(logffmpeg PRIVATE 2702)

target_link_libraries(logffmpeg PRIVATE PkgConfig::AVCODEC)


add_executable(log2mjpeg log2mjpeg/log2mjpeg.cpp)

target_include_directories(log2mjpeg PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(log2mjpeg PRIVATE 2702)

target_link_libraries(log2mjpeg PRIVATE ${OpenCV_LIBS})
target_include_directories(log2mjpeg PRIVATE ${OpenCV_INCLUDE_DIRS})


add_executable(rpi-sample rpi-sample/rpi-sample.cpp)

target_include_directories(rpi-sample PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(rpi-sample PRIVATE 2702)

target_link_libraries(rpi-sample PRIVATE ${OpenCV_LIBS})
target_include_directories(rpi-sample PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(rpi-sample PRIVATE cameraserver::cameraserver)

target_link_libraries(rpi-sample PRIVATE ntcore::ntcore)


add_executable(vid-sample vid-sample/vid.cpp)

target_include_directories(vid-sample PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(vid-sample PRIVATE 2702)

target_link_libraries(vid-sample PRIVATE ${OpenCV_LIBS})
target_include_directories(vid-sample PRIVATE ${OpenCV_INCLUDE_DIRS})