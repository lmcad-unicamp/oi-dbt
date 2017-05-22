cmake_minimum_required (VERSION 2.6)

file(GLOB headers *.h)
file(GLOB sources *.cpp)

add_library(arglib ${sources} ${headers})



