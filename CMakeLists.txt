cmake_minimum_required(VERSION 3.21)
project(HabitTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static linking
set(BUILD_SHARED_LIBS OFF)

# Static MSVC runtime on Windows so users don't need VC++ Redistributable
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Fetch SFML 2.6.2 and build it from source
include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SHALLOW TRUE
)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SFML)

add_executable(HabitTracker
    main.cpp
    orchestrator.cpp
    habitRow.cpp
    habitSquare.cpp
    nameBlock.cpp
)

target_link_libraries(HabitTracker PRIVATE sfml-graphics sfml-window sfml-system)

# On Windows, define SFML_STATIC so headers use correct linkage
if(WIN32)
    target_compile_definitions(HabitTracker PRIVATE SFML_STATIC)
endif()
