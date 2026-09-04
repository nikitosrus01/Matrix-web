cmake_minimum_required(VERSION 3.16)
project(ascii_video_studio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(ascii_studio src/sad.cpp)

target_link_libraries(ascii_studio PRIVATE ${OpenCV_LIBS} pthread)

if(MSVC)
    target_compile_options(ascii_studio PRIVATE /O2 /arch:AVX2)
else()
    target_compile_options(ascii_studio PRIVATE -O3 -march=native -flto)
endif()