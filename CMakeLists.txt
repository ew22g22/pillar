cmake_minimum_required(VERSION 3.20)
project(pillar CXX)
set(CMAKE_CXX_STANDARD 26)

file(GLOB_RECURSE src_files ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(pillar ${src_files})
