cmake_minimum_required(VERSION 4.0)
project(2702Vision)

add_subdirectory(lib) # ** Important: must be first to define library targets prior to initialization of other targets
add_subdirectory(2025/Object)
add_subdirectory(Utilities)
add_subdirectory(tests)