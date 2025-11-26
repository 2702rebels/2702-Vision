add_executable(tests test.cpp)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
target_link_libraries(tests PRIVATE 2702)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/2025/Object/include)

enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)