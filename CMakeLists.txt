cmake_minimum_required(VERSION 3.5)
project(openisa-dbt)

SET(CMAKE_CXX_FLAGS "-Ofast -Werror -std=c++11")

include_directories(include)

add_library(interpreter ITDInterpreter.cpp)
add_library(machine machine.cpp)

add_executable(oi-dbt main.cpp)

target_link_libraries(oi-dbt machine interpreter papi)
