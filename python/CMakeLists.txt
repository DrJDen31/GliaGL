cmake_minimum_required(VERSION 3.15)
project(glia_python)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Find or fetch pybind11
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# Core C++ library (static)
add_library(glia_core STATIC
    ../src/arch/glia.cpp
    ../src/arch/neuron.cpp
    ../src/evo/evolution_engine.cpp
)

# Enable PIC for static library (required for linking into shared library on Linux)
set_target_properties(glia_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(glia_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Python extension module
pybind11_add_module(_core
    src/bind_core.cpp
    src/bind_neuron.cpp
    src/bind_network.cpp
    src/bind_input_sequence.cpp
    src/bind_training.cpp
    src/bind_evolution.cpp
)

target_link_libraries(_core PRIVATE glia_core)

target_include_directories(_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Compiler warnings
if(MSVC)
    target_compile_options(glia_core PRIVATE /W3)
    target_compile_options(_core PRIVATE /W3)
else()
    target_compile_options(glia_core PRIVATE -Wall -Wextra -O3)
    target_compile_options(_core PRIVATE -Wall -Wextra -O3)
endif()

# Install the Python module
install(TARGETS _core
    LIBRARY DESTINATION glia
    RUNTIME DESTINATION glia
)
