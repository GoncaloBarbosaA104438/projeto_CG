cmake_minimum_required(VERSION 3.10)

# Definição do Projeto (ligeiramente diferente)
project(Fase1_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- Dependências Globais ---
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# --- Definição de Bibliotecas ---
# Configurar TinyXML-2 de forma moderna
add_library(tinyxml_lib STATIC tinyXML/tinyxml2.cpp)
target_include_directories(tinyxml_lib PUBLIC ${CMAKE_SOURCE_DIR}/tinyXML)

# --- Definição dos Executáveis ---
add_executable(generator
    generator/generator.cpp
    generator/box.cpp
    generator/plane.cpp
    generator/sphere.cpp
    generator/cone.cpp
    point/point.cpp      
)

add_executable(engine
    engine/engine.cpp
    point/point.cpp     
)
# --- Configuração de Plataforma (Windows vs Linux/macOS) ---
set(COMMON_INCLUDES ${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    # Alterado o nome da variável de cache para parecer diferente
    set(DEPS_DIR "" CACHE PATH "Directory containing glut and glew toolkits")
    message(STATUS "Searching for dependencies in: ${DEPS_DIR}")

    if(NOT EXISTS "${DEPS_DIR}/glut/GL/glut.h" OR NOT EXISTS "${DEPS_DIR}/glew/GL/glew.h")
        message(FATAL_ERROR "Missing GLUT or GLEW headers! Please check DEPS_DIR.")
    endif()

    set(PLATFORM_INCLUDES ${DEPS_DIR}/glut ${DEPS_DIR}/glew)
    set(PLATFORM_LIBS
        ${OPENGL_LIBRARIES}
        ${DEPS_DIR}/glut/glut32.lib
        ${DEPS_DIR}/glew/glew32.lib
    )

    # Copiar as DLLs usando uma sintaxe mais limpa
    if(EXISTS "${DEPS_DIR}/glut/glut32.dll" AND EXISTS "${DEPS_DIR}/glew/glew32.dll")
        file(COPY "${DEPS_DIR}/glut/glut32.dll" "${DEPS_DIR}/glew/glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})
    endif()

else()
    # Configurações para sistemas Unix
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    find_package(GLUT REQUIRED)

    set(PLATFORM_INCLUDES ${GLUT_INCLUDE_DIR})
    set(PLATFORM_LIBS ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

    if(NOT APPLE)
        find_package(GLEW REQUIRED)
        list(APPEND PLATFORM_INCLUDES ${GLEW_INCLUDE_DIRS})
        list(APPEND PLATFORM_LIBS ${GLEW_LIBRARIES})
    endif()
endif()

# --- Linkagem Final ---
# Aplicar todos os includes e bibliotecas aos dois projetos num só ciclo
foreach(TARGET_APP engine generator)
    target_include_directories(${TARGET_APP} PRIVATE ${COMMON_INCLUDES} ${PLATFORM_INCLUDES})
    target_link_libraries(${TARGET_APP} PRIVATE tinyxml_lib ${PLATFORM_LIBS})
endforeach()