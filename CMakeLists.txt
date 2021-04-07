cmake_minimum_required(VERSION 3.16)

project(NaiveJson)

add_library(libnaive naivejson.cpp)

set(CMAKE_CXX_STANDARD 11)

add_executable(NaiveJson naivetest.cpp)

target_link_libraries(NaiveJson libnaive)