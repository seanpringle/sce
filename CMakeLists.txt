cmake_minimum_required(VERSION 3.16)

project(sce)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=jobserver)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO -flto=jobserver)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)

# imgui

set(CMAKE_CXX_STANDARD 11)

add_library(imgui OBJECT imgui/build.cpp)

# sce

set(CMAKE_CXX_STANDARD 17)

add_executable(sce src/main.cc src/config.cc src/theme.cc src/syntax.cc src/project.cc src/view.cc)
include_directories(sce ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(sce imgui ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES} OpenGL::GL GLEW::GLEW stdc++fs)
