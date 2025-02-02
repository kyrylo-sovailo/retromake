cmake_minimum_required(VERSION 3.18)
project(RetroMake LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(RapidJSON REQUIRED)
find_file(RapidXML "rapidxml/rapidxml.hpp" REQUIRED)
get_filename_component(RAPIDXML_INCLUDE_DIRS "${RapidXML}" DIRECTORY)
get_filename_component(RAPIDXML_INCLUDE_DIRS "${RAPIDXML_INCLUDE_DIRS}" DIRECTORY)
message("-- RapidXML found. Headers: ${RAPIDXML_INCLUDE_DIRS}")

add_executable(retromake
source/module_clang.cpp
source/module_codeblocks.cpp
source/module_gcc.cpp
source/module_make.cpp
source/module_native_debug.cpp
source/module_vscode.cpp
source/module_vscodium.cpp
source/module.cpp
source/codemodel.cpp
source/retromake.cpp
source/util.cpp)

target_include_directories(retromake PRIVATE "${RAPIDJSON_INCLUDE_DIRS}")
target_include_directories(retromake PRIVATE "${RAPIDXML_INCLUDE_DIRS}")