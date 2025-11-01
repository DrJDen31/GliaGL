/**
 * @file bind_neuron.cpp
 * @brief Python bindings for Neuron class
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../../src/arch/neuron.h"

namespace py = pybind11;

void bind_neuron(py::module &m) {
    // Neuron class - read-only access for Python
    // Most neuron interaction happens through Network, not directly
    py::class_<Neuron, std::shared_ptr<Neuron>>(m, "Neuron")
        .def("get_id", &Neuron::getId,
             "Get neuron ID")
        .def("get_value", &Neuron::getValue,
             "Get current membrane voltage")
        .def("get_threshold", &Neuron::getThreshold,
             "Get firing threshold")
        .def("set_threshold", &Neuron::setThreshold,
             py::arg("threshold"),
             "Set firing threshold")
        .def("get_leak", &Neuron::getLeak,
             "Get leak parameter")
        .def("set_leak", &Neuron::setLeak,
             py::arg("leak"),
             "Set leak parameter")
        .def("get_resting", &Neuron::getResting,
             "Get resting voltage")
        .def("set_resting", &Neuron::setResting,
             py::arg("resting"),
             "Set resting voltage")
        .def("did_fire", &Neuron::didFire,
             "Check if neuron fired in last timestep")
        .def("__repr__", [](const Neuron &n) {
            return "<Neuron '" + n.getId() + "' threshold=" + 
                   std::to_string(n.getThreshold()) + ">";
        });
}
