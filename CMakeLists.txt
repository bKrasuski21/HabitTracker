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

# --- MSVC runtime -------------------------------------------------------------
# Static CRT, so the shipped .exe runs without the VC++ Redistributable.
#
# This has to be decided before the first target exists. CMAKE_MSVC_RUNTIME_LIBRARY
# is read at add_library()/add_executable() time, so setting it further down the
# file leaves every target created above it on the default /MD and the final link
# fails with LNK2038, plus unresolved __imp__* CRT symbols. Every target in this
# project must agree, so this is the one place that decides.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
    # building it from source, which is what the release workflow does and what
    # anyone building this without SFML installed gets automatically.
    #
    # Not on MSVC, though. We link the static CRT above, a system SFML is built
    # against the DLL one, and mixing them is the LNK2038 this used to ship. On
    # Windows the from-source path is the only supported one.
    if(NOT MSVC)
        find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
    endif()
    if(NOT SFML_FOUND)
        message(STATUS "Building SFML 2.6.2 from source")
        set(BUILD_SHARED_LIBS OFF)
        set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
        set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

        # SFML 2.6.2 declares cmake_minimum_required(VERSION 3.7.2), which puts
        # CMP0091 back to OLD inside its own scope: CMAKE_MSVC_RUNTIME_LIBRARY is
        # ignored there and it would compile /MD no matter what we set. Its own
        # option is the only thing that moves it, and it refuses to combine with
        # BUILD_SHARED_LIBS, which is already off above.
        set(SFML_USE_STATIC_STD_LIBS ON CACHE BOOL "" FORCE)

        include(FetchContent)
        FetchContent_Declare(SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.6.2
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SFML)
    endif()

    # --- Application ----------------------------------------------------------
    file(GLOB HABITTRACKER_APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/view/*.cpp")
    add_executable(HabitTracker ${HABITTRACKER_APP_SOURCES} src/main.cpp)
    target_link_libraries(HabitTracker
        PRIVATE habittracker_core habittracker_warnings sfml-graphics sfml-window sfml-system)
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
