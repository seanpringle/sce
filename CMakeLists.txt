cmake_minimum_required(VERSION 3.16)

project(sce)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=16)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO -flto=16)

#find_package(SDL2 REQUIRED HINTS /home/sean/src/SDL/build)
#find_package(OpenGL REQUIRED)
#find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)
find_package(Threads REQUIRED)

# gtest

set(CMAKE_CXX_STANDARD 11)
set(GTEST googletest/googletest)

add_library(gtest OBJECT EXCLUDE_FROM_ALL ${GTEST}/src/gtest-all.cc ${GTEST}/src/gtest_main.cc)
target_include_directories(gtest PRIVATE ${GTEST} ${GTEST}/include)
target_compile_options(gtest PRIVATE -isystem ${GTEST}/include)

set(CMAKE_CXX_STANDARD 20)

add_executable(sce-test EXCLUDE_FROM_ALL src/test.cc)
target_include_directories(sce-test PRIVATE ${GTEST}/include)
target_link_libraries(sce-test m pthread gtest)

# imgui

set(CMAKE_CXX_STANDARD 11)

add_library(imgui OBJECT imgui/build.cpp)

# sce

set(CMAKE_CXX_STANDARD 20)

add_executable(sce src/main.cc src/config.cc src/theme.cc src/syntax.cc src/project.cc src/view.cc)
include_directories(sce /home/sean/src/SDL/include ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(sce imgui /home/sean/src/SDL/build/.libs/libSDL2.so ${FREETYPE_LIBRARIES} stdc++fs Threads::Threads git2)
