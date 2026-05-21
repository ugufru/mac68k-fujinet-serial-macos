cmake_minimum_required(VERSION 3.9)
project(mac68k_fujinet_retro68 C CXX ASM)

# Top-level build for the Retro68 port of mac68k-fujinet-serial.
#
# Build with:
#   mkdir build && cd build
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=$HOME/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
#   cmake --build .

add_subdirectory(FujiSerialStub)
add_subdirectory(FujiSerialAsync)
add_subdirectory(FujiStubTest)
