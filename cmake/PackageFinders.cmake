file(GLOB_RECURSE PACKAGE_FINDER_FILES "${CMAKE_CURRENT_LIST_DIR}/packagefinders/*.cmake")

foreach(file ${PACKAGE_FINDER_FILES})
    message(STATUS "Adding package finders from ${file}")
    include(${file})
endforeach ()