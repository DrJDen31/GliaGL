/**
 * @file bind_network.cpp
 * @brief Python bindings for Glia (Network) class with NumPy support
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "../../src/arch/glia.h"
#include "../../src/arch/neuron.h"  // Need full definition for shared_ptr in method signatures

namespace py = pybind11;

void bind_network(py::module &m) {
    // Main Network class
    py::class_<Glia, std::shared_ptr<Glia>>(m, "Network",
        "Spiking neural network simulator\n\n"
        "Example:\n"
        "    >>> net = Network(num_sensory=2, num_neurons=5)\n"
        "    >>> net.load('network.net')\n"
        "    >>> net.step()\n")
        
        // Constructors
        .def(py::init<>(),
             "Create empty network")
        .def(py::init<int, int>(),
             py::arg("num_sensory"), py::arg("num_neurons"),
             "Create network with specified neuron counts")
        
        // File I/O
        .def("load", &Glia::configureNetworkFromFile,
             py::arg("filepath"), py::arg("verbose") = true,
             "Load network from .net file")
        .def("save", &Glia::saveNetworkToFile,
             py::arg("filepath"),
             "Save network to .net file")
        
        // Simulation
        .def("step", &Glia::step,
             py::call_guard<py::gil_scoped_release>(),
             "Run one simulation timestep (GIL released)")
        .def("inject", &Glia::injectSensory,
             py::arg("neuron_id"), py::arg("amount"),
             "Inject current into sensory neuron")
        
        // Neuron access
        .def("get_neuron", &Glia::getNeuronById,
             py::arg("neuron_id"),
             "Get neuron by ID")
        .def("get_sensory_ids", &Glia::getSensoryNeuronIDs,
             "Get all sensory neuron IDs")
        .def("get_all_neuron_ids", &Glia::getAllNeuronIDs,
             "Get all neuron IDs (sensory + internal)")
        .def("get_neuron_count", &Glia::getNeuronCount,
             "Get total neuron count")
        .def("get_connection_count", &Glia::getConnectionCount,
             "Get total connection count")
        
        // NumPy-compatible state access
        .def("get_state", [](const Glia &self) {
            std::vector<std::string> ids;
            std::vector<float> values, thresholds, leaks;
            self.getState(ids, values, thresholds, leaks);
            
            // Return NumPy arrays (zero-copy where possible)
            return py::make_tuple(
                ids,
                py::array_t<float>(values.size(), values.data()),
                py::array_t<float>(thresholds.size(), thresholds.data()),
                py::array_t<float>(leaks.size(), leaks.data())
            );
        },
        "Get network state as NumPy arrays\n\n"
        "Returns:\n"
        "    tuple: (ids, values, thresholds, leaks) where values/thresholds/leaks are NumPy arrays")
        
        .def("set_state", [](Glia &self, 
                             const std::vector<std::string> &ids,
                             py::array_t<float> thresholds,
                             py::array_t<float> leaks) {
            // Convert NumPy arrays to vectors
            auto thr_buf = thresholds.request();
            auto leak_buf = leaks.request();
            
            std::vector<float> thr_vec(static_cast<float*>(thr_buf.ptr),
                                       static_cast<float*>(thr_buf.ptr) + thr_buf.size);
            std::vector<float> leak_vec(static_cast<float*>(leak_buf.ptr),
                                        static_cast<float*>(leak_buf.ptr) + leak_buf.size);
            
            self.setState(ids, thr_vec, leak_vec);
        },
        py::arg("ids"), py::arg("thresholds"), py::arg("leaks"),
        "Set neuron parameters from arrays")
        
        // NumPy-compatible weight access (COO sparse format)
        .def("get_weights", [](const Glia &self) {
            std::vector<std::string> from_ids, to_ids;
            std::vector<float> weights;
            self.getWeights(from_ids, to_ids, weights);
            
            return py::make_tuple(
                from_ids,
                to_ids,
                py::array_t<float>(weights.size(), weights.data())
            );
        },
        "Get all synaptic weights as edge list (COO sparse format)\n\n"
        "Returns:\n"
        "    tuple: (from_ids, to_ids, weights) where weights is a NumPy array")
        
        .def("set_weights", [](Glia &self,
                               const std::vector<std::string> &from_ids,
                               const std::vector<std::string> &to_ids,
                               py::array_t<float> weights) {
            // Convert NumPy array to vector
            auto w_buf = weights.request();
            std::vector<float> w_vec(static_cast<float*>(w_buf.ptr),
                                    static_cast<float*>(w_buf.ptr) + w_buf.size);
            
            self.setWeights(from_ids, to_ids, w_vec);
        },
        py::arg("from_ids"), py::arg("to_ids"), py::arg("weights"),
        "Set synaptic weights from edge list (creates connections if needed)")
        
        .def("__repr__", [](const Glia &self) {
            return "<Network neurons=" + std::to_string(self.getNeuronCount()) +
                   " connections=" + std::to_string(self.getConnectionCount()) + ">";
        });
}
