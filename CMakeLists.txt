cmake_minimum_required(VERSION 3.18)
project(RetroMake LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(RapidJSON REQUIRED)
if (RETROMAKE_USE_TIMYXML)
    find_file(TinyXML "tinyxml.h" REQUIRED)
    get_filename_component(TINYXML_INCLUDE_DIRS "${TinyXML}" DIRECTORY)
    message("-- TinyXML found. Headers: ${TINYXML_INCLUDE_DIRS}")
else()
    find_file(RapidXML "rapidxml/rapidxml.hpp" REQUIRED)
    get_filename_component(RAPIDXML_INCLUDE_DIRS "${RapidXML}" DIRECTORY)
    get_filename_component(RAPIDXML_INCLUDE_DIRS "${RAPIDXML_INCLUDE_DIRS}" DIRECTORY)
    message("-- RapidXML found. Headers: ${RAPIDXML_INCLUDE_DIRS}")
endif()

add_executable(retromake
source/codemodel.cpp
source/json.cpp
source/module_clang.cpp
source/module_codeblocks.cpp
source/module_gcc.cpp
source/module_make.cpp
source/module_native_debug.cpp
source/module_vscode.cpp
source/module_vscodium.cpp
source/module.cpp
source/retromake.cpp
source/util.cpp
source/xml.cpp)

target_include_directories(retromake PRIVATE "${RAPIDJSON_INCLUDE_DIRS}")
if (RETROMAKE_USE_TIMYXML)
    target_compile_definitions(retromake PRIVATE TIXML_USE_STL RETROMAKE_USE_TIMYXML)
    target_include_directories(retromake PRIVATE "${TINYXML_INCLUDE_DIRS}")
    target_link_libraries(retromake PRIVATE tinyxml)
else()
    target_include_directories(retromake PRIVATE "${RAPIDXML_INCLUDE_DIRS}")
endif()