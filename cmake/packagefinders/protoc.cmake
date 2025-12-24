# ignore targetname and is_private vars, will always be random values for this one only
function(find_protoc_from_vcpkg targetname is_private)
    find_package(protobuf CONFIG REQUIRED)
    set(PROTOC_EXE "${Protobuf_PROTOC_EXECUTABLE}" CACHE STRING "Path to protobuf compiler")
endfunction()

function(find_protoc_from_sys targetname is_private)
    set(PROTOC_EXE "protoc" CACHE STRING "Path to protobuf compiler")
endfunction()

add_package_finder("protoc" "find_protoc_from_vcpkg" "find_protoc_from_sys")