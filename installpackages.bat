CALL git clone https://github.com/OhmV-IR/vcpkg.git
cd vcpkg
CALL git checkout fmt-update
CALL bootstrap-vcpkg.bat
cd triplets
CALL echo "set(VCPKG_BUILD_TYPE release)" >> x86-windows.cmake
CALL echo "set(VCPKG_BUILD_TYPE release)" >> x64-windows.cmake
CALL echo "set(VCPKG_LIBRARY_LINKAGE static)" >> x86-windows.cmake
CALL echo "set(VCPKG_LIBRARY_LINKAGE static)" >> x64-windows.cmake
cd ..
CALL vcpkg install
CALL rmdir /s /q buildtrees
CALL rmdir /s /q downloads
CALL rmdir /s /q packages
cd ..