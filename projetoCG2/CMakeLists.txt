cmake_minimum_required(VERSION 3.10)
project(Fase1_CG)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_library(GLUT_LIBRARY GLUT)

add_executable(generator generator.cpp)
add_executable(engine engine.cpp tinyxml2.cpp)

target_link_libraries(engine OpenGL::GL ${GLUT_LIBRARY})
