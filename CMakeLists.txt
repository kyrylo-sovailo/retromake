cmake_minimum_required(VERSION 3.5)
project(RetroMake LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(retromake
source/module.cpp
source/module_gcc.cpp
source/module_clang.cpp
source/module_make.cpp
source/module_native_debug.cpp
source/module_vscode.cpp
source/module_vscodium.cpp
source/retromake.cpp
source/util.cpp)