add_library(2702log STATIC implementation/LoggingUtils.cpp)
target_include_directories(2702log PUBLIC include)

add_package_to_target("spdlog" "2702log" FALSE)
target_include_directories(2702log PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)

add_library(udpsender STATIC implementation/UDPSender.cpp)
target_include_directories(udpsender PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
add_package_to_target("asio" "udpsender" FALSE)

set(CONFIG_DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/cfg/processorConfig.json")
set(CONFIG_SRC_FILE "${CMAKE_CURRENT_LIST_DIR}/cfg/processorConfig.json")
add_custom_command(
    OUTPUT "${CONFIG_DEST_FILE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CONFIG_SRC_FILE}" "${CONFIG_DEST_FILE}"
    DEPENDS "${CONFIG_SRC_FILE}"
    COMMENT "Copying object detection processor config"
)

add_custom_target(copyconfigfile DEPENDS "${CONFIG_DEST_FILE}")


set(PROTOBUF_IMPORT_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/proto"
)

# Convert to -I flags
set(PROTOBUF_FLAGS "")
foreach(dir IN LISTS PROTOBUF_IMPORT_DIRS)
    list(APPEND PROTOBUF_FLAGS "-I${dir}")
endforeach()

# Collect all .proto files
file(GLOB_RECURSE PERSISTENCE_PROTOBUF_FILES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

# Generate output files
set(GENERATED_PROTO_SRCS "")
set(GENERATED_PROTO_HDRS "")

add_package_to_target("protoc" "null" FALSE)
foreach(proto_file IN LISTS PACKETS_PROTOBUF_FILES PERSISTENCE_PROTOBUF_FILES)
    get_filename_component(proto_name ${proto_file} NAME_WE)
    set(proto_src_abs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(proto_head_abs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    add_custom_command(
        OUTPUT ${proto_src_abs} ${proto_head_abs}
        COMMAND ${PROTOC_EXE}
        ARGS
        ${PROTOBUF_FLAGS}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${proto_file}
        COMMENT "Compiling protobuf file: ${proto_name}.proto"
    )

    list(APPEND GENERATED_PROTO_SRCS ${proto_src_abs})
    list(APPEND GENERATED_PROTO_HDRS ${proto_head_abs})
endforeach()

add_custom_target(generate_protos DEPENDS ${GENERATED_PROTO_SRCS} ${GENERATED_PROTO_HDRS})

add_library(objectdetectionconfig STATIC implementation/ObjectDetectionConfigValue.cpp implementation/NetworkTopicGeneric.cpp implementation/ObjectDetectionConfig.cpp)
add_dependencies(objectdetectionconfig generate_protos)

target_include_directories(objectdetectionconfig PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_package_to_target("protobuf" "objectdetectionconfig" FALSE)
add_package_to_target("wpilib" "objectdetectionconfig" FALSE)

target_sources(objectdetectionconfig PRIVATE ${GENERATED_PROTO_SRCS})

target_include_directories(objectdetectionconfig PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_library(objectdetectionstruct STATIC implementation/ObjectDetectionStruct.cpp)

target_include_directories(objectdetectionstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_package_to_target("wpilib" "objectdetectionstruct" FALSE)

add_library(objectframeprocessor STATIC implementation/ObjectFrameProcessor.cpp)

target_include_directories(objectframeprocessor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(objectframeprocessor PRIVATE 2702log)

target_link_libraries(objectframeprocessor PRIVATE objectdetectionstruct)

target_link_libraries(objectframeprocessor PUBLIC objectdetectionconfig)

target_link_libraries(objectframeprocessor PUBLIC udpsender)

target_link_libraries(objectframeprocessor PRIVATE 2702)
target_include_directories(objectframeprocessor PUBLIC ${CMAKE_SOURCE_DIR}/lib/include)

add_package_to_target("opencv" "objectframeprocessor" TRUE)
add_package_to_target("wpilib" "objectframeprocessor" TRUE)

add_dependencies(objectframeprocessor copyconfigfile)
target_compile_definitions(objectframeprocessor PRIVATE CONFIG_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}/cfg")


add_executable(calibrate implementation/calibrate.cpp)
target_include_directories(calibrate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_package_to_target("opencv" "calibrate" TRUE)

target_link_libraries(calibrate PRIVATE 2702log)

add_package_to_target("json" "calibrate" TRUE)

add_executable(calib_deviation implementation/calib_deviation.cpp)
target_include_directories(calib_deviation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_package_to_target("json" "calib_deviation" TRUE)
target_link_libraries(calib_deviation PRIVATE 2702log)

if(IS_RPI)
	add_executable(object ${CMAKE_CURRENT_SOURCE_DIR}/implementation/object.cpp)
	target_link_libraries(object PRIVATE objectframeprocessor)
    add_package_to_target("libcamera" "object" TRUE)
endif()


add_executable(nettableserver implementation/nettableserver.cpp)

target_include_directories(nettableserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_package_to_target("wpilib" "nettableserver" TRUE)

target_link_libraries(nettableserver PRIVATE 2702log)


add_executable(nettableclient implementation/nettableclient.cpp)

target_include_directories(nettableclient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_package_to_target("wpilib" "nettableclient" TRUE)

target_link_libraries(nettableclient PRIVATE 2702log)


add_executable(testudpserver implementation/testudpserver.cpp)

target_include_directories(testudpserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(testudpserver PRIVATE 2702log)

target_link_libraries(testudpserver PRIVATE objectdetectionstruct)

add_package_to_target("asio" "testudpserver" TRUE)

add_library(ffmpeglogger STATIC implementation/FFMPEGLogger.cpp)
add_package_to_target("ffmpeg" "ffmpeglogger" FALSE)
add_package_to_target("opencv" "ffmpeglogger" FALSE)
target_include_directories(ffmpeglogger PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(logtostdvideo implementation/logtostdvideo.cpp)

target_link_libraries(logtostdvideo PRIVATE objectframeprocessor)
target_link_libraries(logtostdvideo PRIVATE 2702log)
target_link_libraries(logtostdvideo PRIVATE ffmpeglogger)

add_executable(replay implementation/replay.cpp)

target_link_libraries(replay PRIVATE objectframeprocessor)
target_link_libraries(replay PRIVATE 2702log)
