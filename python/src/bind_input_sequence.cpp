/**
 * @file bind_input_sequence.cpp
 * @brief Python bindings for InputSequence class
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../../src/arch/input_sequence.h"

namespace py = pybind11;

void bind_input_sequence(py::module &m) {
    // InputSequence class
    py::class_<InputSequence>(m, "InputSequence",
        "Input sequence for episodes\n\n"
        "Represents a temporal sequence of sensory inputs.")
        
        .def(py::init<>(),
             "Create empty input sequence")
        
        .def("reset", &InputSequence::reset,
             "Reset sequence to beginning")
        
        .def("advance", &InputSequence::advance,
             "Advance to next timestep")
        
        .def("get_current_inputs", &InputSequence::getCurrentInputs,
             "Get current timestep inputs as dict")
        
        .def("load_from_file", &InputSequence::loadFromFile,
             py::arg("filepath"),
             "Load sequence from .seq file")
        
        .def("add_timestep", [](InputSequence &self, const std::map<std::string, float> &inputs) {
            // Get the next available tick
            // If empty, start at 0; otherwise max + 1
            int tick = self.isEmpty() ? 0 : (self.getMaxTick() + 1);
            // Add each input as an event at this tick
            for (const auto &pair : inputs) {
                self.addEvent(tick, pair.first, pair.second);
            }
        },
        py::arg("inputs"),
        "Add a timestep with input values")
        
        .def("__repr__", [](const InputSequence &self) {
            return "<InputSequence>";
        });
}
