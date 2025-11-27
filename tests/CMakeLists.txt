find_package(OpenCV REQUIRED)
find_package(wpilib REQUIRED)
find_package(GTest REQUIRED)

enable_testing()
include(GoogleTest)

add_executable(tests)

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/implementation/*.cpp")
target_sources(tests PRIVATE ${TEST_SOURCES})

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(tests PRIVATE 2702)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/2025/Object/include)

target_link_libraries(tests PRIVATE objectframeprocessor)

target_link_libraries(tests PRIVATE gtest gtest_main)

gtest_discover_tests(tests)