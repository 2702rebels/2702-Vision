find_package(OpenCV REQUIRED)
find_package(wpilib REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED)
pkg_check_modules(libcamera REQUIRED IMPORTED_TARGET libcamera)

add_executable(tests)

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/implementation/*.cpp")
target_sources(tests PRIVATE ${TEST_SOURCES})

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(tests PRIVATE 2702)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/2025/Object/include)

target_include_directories(tests PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tests PRIVATE ${OpenCV_LIBS})

target_link_libraries(tests PRIVATE apriltag wpilibNewCommands wpilibc cameraserver cscore ntcore wpinet wpimath wpiutil)

target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main)

target_link_libraries(tests PRIVATE PkgConfig::libcamera)

enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)