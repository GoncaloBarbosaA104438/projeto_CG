cmake_minimum_required(VERSION 3.10)
project(Fase1_CG)

set(CMAKE_CXX_STANDARD 17)

# Resolve o aviso CMP0072 (Diz ao CMake para usar o novo sistema GLVND do Linux)
set(OpenGL_GL_PREFERENCE GLVND)

# Encontra o OpenGL (GL e GLU)
find_package(OpenGL REQUIRED)

# Encontra o GLUT da forma correta
find_package(GLUT REQUIRED)

# Executáveis
add_executable(generator generator.cpp)
add_executable(engine engine.cpp tinyxml2.cpp)

# Linkar o engine com GL, GLU e GLUT (nota que aqui é GLUT_LIBRARIES, no plural)
target_link_libraries(engine OpenGL::GL OpenGL::GLU ${GLUT_LIBRARIES})

# Se o generator não usar OpenGL, esta linha abaixo não é precisa. 
# Se usar, podes descomentá-la:
# target_link_libraries(generator OpenGL::GL OpenGL::GLU ${GLUT_LIBRARIES})