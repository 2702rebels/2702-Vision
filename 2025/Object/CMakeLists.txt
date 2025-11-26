find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)
find_package(wpilib REQUIRED)

add_library(2702log STATIC implementation/LoggingUtils.cpp)
target_include_directories(2702log PRIVATE include)

target_include_directories(2702log PUBLIC ${spdlog_INCLUDE_DIRS})
target_link_libraries(2702log PUBLIC ${spdlog_LIBS})

target_include_directories(2702log PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(2702log PUBLIC ${OpenCV_LIBS})

target_include_directories(2702log PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)


add_executable(calibrate implementation/calibrate.cpp)
target_include_directories(calibrate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(calibrate PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calibrate PRIVATE ${OpenCV_LIBS})

target_link_libraries(calibrate PRIVATE 2702log)

if(IS_RPI)
	add_executable(object ${CMAKE_CURRENT_SOURCE_DIR}/implementation/object.cpp)
	target_include_directories(object PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

add_library(objectdetectionstruct STATIC implementation/ObjectDetectionStruct.cpp)

target_include_directories(objectdetectionstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(objectdetectionstruct PRIVATE wpiutil)


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