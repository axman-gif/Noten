cmake_minimum_required(VERSION 3.20)
project(Noten LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configuración de Raylib nativa
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(raylib)

set(SOURCES
    src/main.cpp
    src/app.cpp
    src/editor.cpp
    src/language_model.cpp
    src/glossary.cpp
    src/renderer.cpp
    src/ui.cpp
    src/persistence.cpp
    src/text_utils.cpp
)

set(HEADERS
    include/app.hpp
    include/editor.hpp
    include/language_model.hpp
    include/glossary.hpp
    include/renderer.hpp
    include/ui.hpp
    include/persistence.hpp
    include/text_utils.hpp
)

add_executable(Noten WIN32 ${SOURCES} ${HEADERS})

target_include_directories(Noten PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${raylib_SOURCE_DIR}/src
)

target_link_libraries(Noten PRIVATE raylib)

if(WIN32)
    target_link_libraries(Noten PRIVATE winmm gdi32 opengl32 shell32)
endif()
