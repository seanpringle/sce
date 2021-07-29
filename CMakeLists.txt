cmake_minimum_required(VERSION 3.16)

project(sce)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -g")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=jobserver)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO -flto=jobserver)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# imgui

set(CMAKE_CXX_STANDARD 11)

add_library(imgui OBJECT imgui/build.cpp)

# sce

set(CMAKE_CXX_STANDARD 17)

add_library(config OBJECT src/config.cc)
add_library(theme OBJECT src/theme.cc)
add_library(syntax OBJECT src/syntax.cc)
add_library(view OBJECT src/view.cc)

add_executable(sce src/main.cc)
include_directories(sce ${SDL2_INCLUDE_DIRS})
target_link_libraries(sce imgui config theme syntax view ${SDL2_LIBRARIES} OpenGL::GL GLEW::GLEW stdc++fs)
