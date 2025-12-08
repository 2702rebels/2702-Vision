find_package(OpenCV REQUIRED)
find_package(wpilib REQUIRED)
find_package(PkgConfig)
pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET libavcodec libavutil libavformat)

add_executable(logffmpeg logffmpeg/logffmpeg.cpp)

target_include_directories(logffmpeg PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(logffmpeg PRIVATE 2702)

target_link_libraries(logffmpeg PRIVATE PkgConfig::ffmpeg)

target_link_libraries(logffmpeg PRIVATE ${OpenCV_LIBS})
target_include_directories(logffmpeg PRIVATE ${OpenCV_INCLUDE_DIRS})


add_executable(log2mjpeg log2mjpeg/log2mjpeg.cpp)

target_include_directories(log2mjpeg PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(log2mjpeg PRIVATE 2702)

target_link_libraries(log2mjpeg PRIVATE ${OpenCV_LIBS})
target_include_directories(log2mjpeg PRIVATE ${OpenCV_INCLUDE_DIRS})

if(IS_RPI)
	add_executable(rpi-sample rpi-sample/rpi-sample.cpp)

	target_include_directories(rpi-sample PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
	target_link_libraries(rpi-sample PRIVATE 2702)

	target_link_libraries(rpi-sample PRIVATE ${OpenCV_LIBS})
	target_include_directories(rpi-sample PRIVATE ${OpenCV_INCLUDE_DIRS})

	target_link_libraries(rpi-sample PRIVATE cameraserver ntcore cscore wpiutil)

	pkg_check_modules(libcamera REQUIRED IMPORTED_TARGET libcamera)
	target_link_libraries(rpi-sample PRIVATE PkgConfig::libcamera)
endif()


add_executable(vid-sample vid-sample/vid.cpp)

target_include_directories(vid-sample PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(vid-sample PRIVATE 2702)

target_link_libraries(vid-sample PRIVATE ${OpenCV_LIBS})
target_include_directories(vid-sample PRIVATE ${OpenCV_INCLUDE_DIRS})

add_executable(v1logtov2 v1logtov2/v1logtov2.cpp)

target_link_libraries(v1logtov2 PRIVATE 2702)
target_include_directories(v1logtov2 PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)

target_include_directories(v1logtov2 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(v1logtov2 PRIVATE ${OpenCV_LIBS})