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

# Digits (.seq) trainer example
add_executable(glia_digits_seq
  ../../examples/seq_digits_poisson/main.cpp
  ../arch/glia.cpp
  ../arch/neuron.cpp
)

target_include_directories(glia_digits_seq PRIVATE ../arch ../train ../../examples/seq_digits_poisson)

if (MSVC)
  target_compile_options(glia_digits_seq PRIVATE /W4)
else()
  target_compile_options(glia_digits_seq PRIVATE -Wall -Wextra -O2)
endif()

# 3-class evolution runner
add_executable(glia_3class_evo
  ../../examples/3class/evaluator/three_class_evo_main.cpp
  ../arch/glia.cpp
  ../arch/neuron.cpp
  ../evo/evolution_engine.cpp
)

target_include_directories(glia_3class_evo PRIVATE ../arch ../train ../evo ../../examples/3class/evaluator)

if (MSVC)
  target_compile_options(glia_3class_evo PRIVATE /W4)
else()
  target_compile_options(glia_3class_evo PRIVATE -Wall -Wextra -O2)
endif()
# Mini-world evolution runner (uses src/evo/EvolutionEngine)
add_executable(glia_miniworld_evo
  ../../examples/mini-world/evaluator/mini_world_evo_main.cpp
  ../arch/glia.cpp
  ../arch/neuron.cpp
  ../evo/evolution_engine.cpp
)

target_include_directories(glia_miniworld_evo PRIVATE ../arch ../train ../evo ../../examples/mini-world/evaluator)

if (MSVC)
  target_compile_options(glia_miniworld_evo PRIVATE /W4)
else()
  target_compile_options(glia_miniworld_evo PRIVATE -Wall -Wextra -O2)
endif()
