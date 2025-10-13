cmake_minimum_required(VERSION 3.10)
project(glia_eval)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(glia_eval
  eval_main.cpp
  ../arch/glia.cpp
  ../arch/neuron.cpp
)

target_include_directories(glia_eval PRIVATE ../arch ../train)

if (MSVC)
  target_compile_options(glia_eval PRIVATE /W4)
else()
  target_compile_options(glia_eval PRIVATE -Wall -Wextra -O2)
endif()

# Mini-world training runner
add_executable(glia_miniworld
  mini_world_main.cpp
  ../arch/glia.cpp
  ../arch/neuron.cpp
)

target_include_directories(glia_miniworld PRIVATE ../arch ../train)

if (MSVC)
  target_compile_options(glia_miniworld PRIVATE /W4)
else()
  target_compile_options(glia_miniworld PRIVATE -Wall -Wextra -O2)
endif()
