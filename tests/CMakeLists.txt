find_package(OpenCV REQUIRED)
find_package(wpilib REQUIRED)

add_executable(tests test.cpp)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(tests PRIVATE 2702)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/2025/Object/include)

target_include_directories(tests PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tests PRIVATE ${OpenCV_LIBS})

target_link_libraries(tests PRIVATE apriltag wpilibNewCommands wpilibc cameraserver cscore ntcore wpinet wpimath wpiutil)

enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)