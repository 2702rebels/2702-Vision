add_executable(cameracalib CameraCalib.cpp)

add_package_to_target("cserialport" "cameracalib" TRUE)
add_package_to_target("opencv" "cameracalib" TRUE)