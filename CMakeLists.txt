cmake_minimum_required(VERSION 3.10)

project(sce)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -g")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=jobserver)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO -flto=jobserver)

file(GLOB SRC_H CONFIGURE_DEPENDS "src/*.h")
file(GLOB SRC_MAIN CONFIGURE_DEPENDS "src/*.cc")

set(CMAKE_CXX_STANDARD 17)
add_executable(ed1 ${SRC_H} ${SRC_MAIN})
target_link_libraries(ed1 m pthread dl stdc++fs)
