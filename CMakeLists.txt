cmake_minimum_required(VERSION 4.0)
project(RayTracer)

set(CMAKE_CXX_STANDARD 20)

add_executable(RayTracer src/main.cpp libraries/glad.c
        src/Shader.cpp
        src/input/KeyboardInput.cpp
        src/input/MouseInput.cpp)
target_include_directories(RayTracer PRIVATE ${CMAKE_SOURCE_DIR}/libraries/include ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(RayTracer ${CMAKE_SOURCE_DIR}/libraries/libs/libglfw3.a)
