cmake_minimum_required(VERSION 3.0)
project(yieldemuduo)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17")

aux_source_directory(. SRC_LIST)

add_library(yieldemuduo SHARED ${SRC_LIST})