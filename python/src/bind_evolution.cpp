/**
 * @file bind_evolution.cpp
 * @brief Python bindings for evolutionary training
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "../../src/evo/evolution_engine.h"

namespace py = pybind11;

void bind_evolution(py::module &m) {
    // EvoMetrics struct
    py::class_<EvoMetrics>(m, "EvoMetrics",
        "Metrics from evolutionary training")
        .def(py::init<>())
        .def_readwrite("fitness", &EvoMetrics::fitness, "Overall fitness score")
        .def_readwrite("acc", &EvoMetrics::acc, "Classification accuracy")
        .def_readwrite("margin", &EvoMetrics::margin, "Average margin")
        .def_readwrite("edges", &EvoMetrics::edges, "Number of connections")
        .def("__repr__", [](const EvoMetrics &m) {
            return "<EvoMetrics fitness=" + std::to_string(m.fitness) +
                   " acc=" + std::to_string(m.acc) + ">";
        });
    
    // EvolutionEngine::Config
    py::class_<EvolutionEngine::Config>(m, "EvolutionConfig",
        "Configuration for evolutionary training")
        .def(py::init<>())
        .def_readwrite("population", &EvolutionEngine::Config::population)
        .def_readwrite("generations", &EvolutionEngine::Config::generations)
        .def_readwrite("elite", &EvolutionEngine::Config::elite)
        .def_readwrite("parents_pool", &EvolutionEngine::Config::parents_pool)
        .def_readwrite("train_epochs", &EvolutionEngine::Config::train_epochs)
        .def_readwrite("sigma_w", &EvolutionEngine::Config::sigma_w,
                      "Weight mutation std dev")
        .def_readwrite("sigma_thr", &EvolutionEngine::Config::sigma_thr,
                      "Threshold mutation std dev")
        .def_readwrite("sigma_leak", &EvolutionEngine::Config::sigma_leak,
                      "Leak mutation std dev")
        .def_readwrite("w_acc", &EvolutionEngine::Config::w_acc,
                      "Accuracy weight in fitness")
        .def_readwrite("w_margin", &EvolutionEngine::Config::w_margin,
                      "Margin weight in fitness")
        .def_readwrite("w_sparsity", &EvolutionEngine::Config::w_sparsity,
                      "Sparsity penalty weight")
        .def_readwrite("seed", &EvolutionEngine::Config::seed)
        .def_readwrite("lamarckian", &EvolutionEngine::Config::lamarckian)
        .def_readwrite("lineage_json", &EvolutionEngine::Config::lineage_json)
        .def("__repr__", [](const EvolutionEngine::Config &c) {
            return "<EvolutionConfig pop=" + std::to_string(c.population) +
                   " gens=" + std::to_string(c.generations) + ">";
        });
    
    // NetworkSnapshot structs
    py::class_<EvolutionEngine::NeuronRec>(m, "NeuronRecord")
        .def(py::init<>())
        .def_readwrite("id", &EvolutionEngine::NeuronRec::id)
        .def_readwrite("thr", &EvolutionEngine::NeuronRec::thr)
        .def_readwrite("leak", &EvolutionEngine::NeuronRec::leak);
    
    py::class_<EvolutionEngine::EdgeRec>(m, "EdgeRecord")
        .def(py::init<>())
        .def_readwrite("from", &EvolutionEngine::EdgeRec::from)
        .def_readwrite("to", &EvolutionEngine::EdgeRec::to)
        .def_readwrite("w", &EvolutionEngine::EdgeRec::w);
    
    py::class_<EvolutionEngine::NetSnapshot>(m, "NetworkSnapshot")
        .def(py::init<>())
        .def_readwrite("neurons", &EvolutionEngine::NetSnapshot::neurons)
        .def_readwrite("edges", &EvolutionEngine::NetSnapshot::edges);
    
    // Evolution Result
    py::class_<EvolutionEngine::Result>(m, "EvolutionResult",
        "Result from evolutionary training")
        .def(py::init<>())
        .def_readwrite("best_genome", &EvolutionEngine::Result::best_genome)
        .def_readwrite("best_fitness_hist", &EvolutionEngine::Result::best_fitness_hist)
        .def_readwrite("best_acc_hist", &EvolutionEngine::Result::best_acc_hist)
        .def_readwrite("best_margin_hist", &EvolutionEngine::Result::best_margin_hist);
    
    // Callbacks struct (bind before EvolutionEngine to avoid default arg issues)
    py::class_<EvolutionEngine::Callbacks>(m, "EvolutionCallbacks")
        .def(py::init<>());
    
    // EvolutionEngine class
    py::class_<EvolutionEngine, std::shared_ptr<EvolutionEngine>>(m, "EvolutionEngine",
        "Evolutionary trainer with Lamarckian evolution\n\n"
        "Example:\n"
        "    >>> evo = EvolutionEngine(net_path, train_data, val_data, train_cfg, evo_cfg)\n"
        "    >>> result = evo.run()\n")
        
        // Constructor without callbacks (most common)
        .def(py::init<const std::string&,
                      const std::vector<Trainer::EpisodeData>&,
                      const std::vector<Trainer::EpisodeData>&,
                      const TrainingConfig&,
                      const EvolutionEngine::Config&>(),
             py::arg("network_path"),
             py::arg("train_set"),
             py::arg("val_set"),
             py::arg("train_config"),
             py::arg("evo_config"),
             "Create evolution engine")
        
        // Constructor with callbacks (advanced)
        .def(py::init<const std::string&,
                      const std::vector<Trainer::EpisodeData>&,
                      const std::vector<Trainer::EpisodeData>&,
                      const TrainingConfig&,
                      const EvolutionEngine::Config&,
                      const EvolutionEngine::Callbacks&>(),
             py::arg("network_path"),
             py::arg("train_set"),
             py::arg("val_set"),
             py::arg("train_config"),
             py::arg("evo_config"),
             py::arg("callbacks"),
             "Create evolution engine with custom callbacks")
        
        .def("run", &EvolutionEngine::run,
             py::call_guard<py::gil_scoped_release>(),
             "Run evolutionary training (GIL released)\n\n"
             "Note: This releases the GIL for the duration.\n"
             "For Python callbacks, use the Python wrapper in glia.evolution")
        
        .def("__repr__", [](const EvolutionEngine &e) {
            return "<EvolutionEngine>";
        });
}
