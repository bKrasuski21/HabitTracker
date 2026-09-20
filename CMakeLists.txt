cmake_minimum_required(VERSION 3.21)
project(HabitTracker VERSION 2.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(HABITTRACKER_BUILD_TESTS "Build the test suite" ON)
option(HABITTRACKER_BUILD_APP "Build the GUI application (requires SFML)" ON)
option(HABITTRACKER_WERROR "Treat warnings as errors" OFF)

# --- Warnings -----------------------------------------------------------------
# An interface target so every one of our own targets gets the same flags, and
# fetched dependencies get none of them.
add_library(habittracker_warnings INTERFACE)
if(MSVC)
    target_compile_options(habittracker_warnings INTERFACE /W4 /permissive-)
    if(HABITTRACKER_WERROR)
        target_compile_options(habittracker_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(habittracker_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wdouble-promotion)
    if(HABITTRACKER_WERROR)
        target_compile_options(habittracker_warnings INTERFACE -Werror)
    endif()
endif()

# --- Core library -------------------------------------------------------------
# The model, its serialization, and its storage. No SFML, so the whole thing is
# testable without a window.
#
# CONFIGURE_DEPENDS re-globs on build. Listing sources explicitly is usually
# better practice, but this project also ships a Makefile, and the two lists
# drifting apart is a bug this repository has already had once.
file(GLOB HABITTRACKER_CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp")
add_library(habittracker_core STATIC ${HABITTRACKER_CORE_SOURCES})
target_include_directories(habittracker_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(habittracker_core PUBLIC cxx_std_20)
target_link_libraries(habittracker_core PRIVATE habittracker_warnings)

# --- SFML and the application -------------------------------------------------
# Both are optional so the model and its tests build with no dependencies at all,
# which is what CI does on every platform.
if(HABITTRACKER_BUILD_APP)
    # Prefer an installed SFML so a clean build needs no network; fall back to
    # building it from source, which is what the release workflow does.
    find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
    if(NOT SFML_FOUND)
        message(STATUS "SFML not found locally; fetching 2.6.2")
        set(BUILD_SHARED_LIBS OFF)
        if(MSVC)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
        set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
        set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

        include(FetchContent)
        FetchContent_Declare(SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.6.2
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SFML)
        set(HABITTRACKER_SFML_STATIC ON)
    endif()

    # --- Application ----------------------------------------------------------
    file(GLOB HABITTRACKER_APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/view/*.cpp")
    add_executable(HabitTracker ${HABITTRACKER_APP_SOURCES} src/main.cpp)
    target_link_libraries(HabitTracker
        PRIVATE habittracker_core habittracker_warnings sfml-graphics sfml-window sfml-system)

    if(WIN32 AND HABITTRACKER_SFML_STATIC)
        target_compile_definitions(HabitTracker PRIVATE SFML_STATIC)
    endif()
endif()  # HABITTRACKER_BUILD_APP

# --- Tests --------------------------------------------------------------------
if(HABITTRACKER_BUILD_TESTS)
    enable_testing()
    file(GLOB HABITTRACKER_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    add_executable(habittracker_tests ${HABITTRACKER_TEST_SOURCES})
    target_include_directories(habittracker_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(habittracker_tests PRIVATE habittracker_core habittracker_warnings)
    add_test(NAME habittracker_tests COMMAND habittracker_tests)
endif()
