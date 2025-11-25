find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)

add_library(2702log STATIC LoggingUtils.cpp)
target_link_libraries(2702log PUBLIC spdlog::spdlog)

add_executable(calibrate calibrate.cpp)
target_include_directories(calibrate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(calibrate PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calibrate PRIVATE ${OpenCV_LIBS})

target_link_libraries(calibrate PRIVATE 2702log)

if(IS_RPI)
	add_executable(object ${CMAKE_CURRENT_SOURCE_DIR}/implementation/object.cpp)
	target_include_directories(object PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()