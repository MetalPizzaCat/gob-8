cmake_minimum_required(VERSION 3.10.0)
project(gob-8 VERSION 0.1.0 LANGUAGES C CXX)

add_compile_definitions(TOTAL_VIDEO_MEMORY_SIZE=64*32)
add_compile_definitions(TOTAL_MEMORY_SIZE=0x1000)

find_package(SDL2 REQUIRED)
include_directories(gob-8t ${SDL2_INCLUDE_DIRS})
add_executable(gob-8 main.cpp
Display.hpp
Display.cpp
DisplaySDL.hpp
DisplaySDL.cpp
Machine.hpp
Machine.cpp)
target_link_libraries(gob-8 ${SDL2_LIBRARIES})


