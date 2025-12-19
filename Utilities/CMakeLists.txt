add_executable(logffmpeg logffmpeg/logffmpeg.cpp)

target_include_directories(logffmpeg PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(logffmpeg PRIVATE 2702)

add_package_to_target("ffmpeg" "logffmpeg" TRUE)
add_package_to_target("opencv" "logffmpeg" TRUE)


add_executable(log2mjpeg log2mjpeg/log2mjpeg.cpp)

target_include_directories(log2mjpeg PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(log2mjpeg PRIVATE 2702)

add_package_to_target("opencv" "log2mjpeg" TRUE)


if(IS_RPI)
	add_executable(rpi-sample rpi-sample/rpi-sample.cpp)

	target_include_directories(rpi-sample PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
	target_link_libraries(rpi-sample PRIVATE 2702)

    add_package_to_target("opencv" "rpi-sample" TRUE)
	add_package_to_target("wpilib" "rpi-sample" TRUE)
    add_package_to_target("libcamera" "rpi-sample" TRUE)
endif()

if(IS_RPI)
add_executable(vid-sample vid-sample/vid.cpp)

target_include_directories(vid-sample PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(vid-sample PRIVATE 2702)

add_package_to_target("opencv" "vid-sample" TRUE)
endif ()


add_executable(v1logtov2 v1logtov2/v1logtov2.cpp)

target_link_libraries(v1logtov2 PRIVATE 2702)
target_include_directories(v1logtov2 PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)

add_package_to_target("opencv" "v1logtov2" TRUE)