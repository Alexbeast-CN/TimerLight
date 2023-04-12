cmake_minimum_required(VERSION 3.16)
project(elevator)

set(CMAKE_CXX_STANDARD 17)

add_executable(test TimerToggle.cpp)
target_link_libraries(test pthread)

