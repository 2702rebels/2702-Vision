
add_library(2702 STATIC)

file(GLOB_RECURSE 2702_SOURCES "implementation/*")
target_sources(2702 PRIVATE ${2702_SOURCES})

target_include_directories(2702 PUBLIC "include")