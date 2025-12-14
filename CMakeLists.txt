# CMakeLists.txt for DrumBox project
cmake_minimum_required(VERSION 3.20) # Défini la version minimale de CMake requise
project(DrumBox LANGUAGES CXX) # Défini le nom du projet et le langage utilisé

set(CMAKE_CXX_STANDARD 17) # Défini la norme C++ à utiliser
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Rend la norme C++ obligatoire
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Génère le fichier compile_commands.json

# Core library
add_library(drumbox_core
    core/src/Engine.cpp
)

# 
target_include_directories(drumbox_core PUBLIC
    ${CMAKE_SOURCE_DIR}/core/include
)

add_executable(main_test
    main_test.cpp
)

target_link_libraries(main_test PRIVATE drumbox_core)

add_subdirectory(tools/runner_miniaudio)
