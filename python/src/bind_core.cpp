/**
 * @file bind_core.cpp
 * @brief Main pybind11 binding file for GliaGL
 * 
 * This file creates the Python module '_core' which exposes the C++ classes.
 * The Python package 'glia' will wrap this for a cleaner API.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

// Include C++ headers
#include "../../src/arch/glia.h"
#include "../../src/arch/neuron.h"
#include "../../src/arch/input_sequence.h"
#include "../../src/arch/output_detection.h"
#include "../../src/train/trainer.h"
#include "../../src/train/training_config.h"
#include "../../src/evo/evolution_engine.h"

namespace py = pybind11;

// Forward declarations for binding functions
void bind_neuron(py::module &m);
void bind_network(py::module &m);
void bind_input_sequence(py::module &m);
void bind_training(py::module &m);
void bind_evolution(py::module &m);

PYBIND11_MODULE(_core, m) {
    m.doc() = "GliaGL C++ core module - Fast spiking neural network simulator";
    
    // Version info
    m.attr("__version__") = "0.1.0";
    
    // Bind components
    bind_neuron(m);
    bind_network(m);
    bind_input_sequence(m);
    bind_training(m);
    bind_evolution(m);
}
