cmake_minimum_required(VERSION 3.18)
project(Example LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(shared SHARED shared.cpp)
add_library(static STATIC static.cpp)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE shared static)