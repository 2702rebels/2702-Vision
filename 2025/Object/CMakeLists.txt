find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)
find_package(wpilib REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf)

add_library(2702log STATIC implementation/LoggingUtils.cpp)
target_include_directories(2702log PRIVATE include)

target_include_directories(2702log PUBLIC ${spdlog_INCLUDE_DIRS})
target_link_libraries(2702log PUBLIC ${spdlog_LIBS})

target_include_directories(2702log PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(2702log PUBLIC ${OpenCV_LIBS})

target_include_directories(2702log PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)

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

foreach(proto_file IN LISTS PACKETS_PROTOBUF_FILES PERSISTENCE_PROTOBUF_FILES)
    get_filename_component(proto_name ${proto_file} NAME_WE)
    set(proto_src_abs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(proto_head_abs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    add_custom_command(
        OUTPUT ${proto_src_abs} ${proto_head_abs}
        COMMAND protoc
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

add_library(objectdetectionconfig STATIC implementation/ObjectDetectionConfigValue.cpp implementation/NetworkTopicPoller.cpp implementation/ObjectDetectionConfig.cpp)
add_dependencies(objectdetectionconfig generate_protos)

target_include_directories(objectdetectionconfig PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(objectdetectionconfig PUBLIC PkgConfig::protobuf)

target_link_libraries(objectdetectionconfig PUBLIC ntcore)

target_sources(objectdetectionconfig PRIVATE ${GENERATED_PROTO_SRCS})

target_include_directories(objectdetectionconfig PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_library(objectdetectionstruct STATIC implementation/ObjectDetectionStruct.cpp)

target_include_directories(objectdetectionstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(objectdetectionstruct PUBLIC wpiutil)

add_library(objectframeprocessor STATIC implementation/ObjectFrameProcessor.cpp)

target_include_directories(objectframeprocessor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(objectframeprocessor PRIVATE 2702log)

target_link_libraries(objectframeprocessor PRIVATE objectdetectionstruct)

target_link_libraries(objectframeprocessor PUBLIC objectdetectionconfig)

target_link_libraries(objectframeprocessor PRIVATE 2702)
target_include_directories(objectframeprocessor PUBLIC ${CMAKE_SOURCE_DIR}/lib/include)

target_include_directories(objectframeprocessor PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(objectframeprocessor PRIVATE ${OpenCV_LIBS})

target_link_libraries(objectframeprocessor PUBLIC wpilibNewCommands wpilibc cameraserver cscore ntcore wpinet wpimath wpiutil)



add_executable(calibrate implementation/calibrate.cpp)
target_include_directories(calibrate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(calibrate PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calibrate PRIVATE ${OpenCV_LIBS})

target_link_libraries(calibrate PRIVATE 2702log)

if(IS_RPI)
	add_executable(object ${CMAKE_CURRENT_SOURCE_DIR}/implementation/object.cpp)
	target_link_libraries(object PRIVATE objectframeprocessor)
endif()


add_executable(nettableserver implementation/nettableserver.cpp)

target_include_directories(nettableserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(nettableserver PRIVATE ntcore wpinet wpiutil)

target_link_libraries(nettableserver PRIVATE 2702log)


add_executable(nettableclient implementation/nettableclient.cpp)

target_include_directories(nettableclient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(nettableclient PRIVATE ntcore wpinet wpiutil)

target_link_libraries(nettableclient PRIVATE 2702log)


add_executable(testudpserver implementation/testudpserver.cpp)

target_include_directories(testudpserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(testudpserver PRIVATE 2702log)

target_link_libraries(testudpserver PRIVATE objectdetectionstruct)

target_include_directories(testudpserver PRIVATE /usr/include/asio)


add_executable(logtostdvideo implementation/logtostdvideo.cpp)

target_link_libraries(logtostdvideo PRIVATE objectframeprocessor)
target_link_libraries(logtostdvideo PRIVATE 2702log)

add_executable(replay implementation/replay.cpp)

target_link_libraries(replay PRIVATE objectframeprocessor)
target_link_libraries(replay PRIVATE 2702log)