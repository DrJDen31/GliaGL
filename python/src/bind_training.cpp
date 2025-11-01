/**
 * @file bind_training.cpp
 * @brief Python bindings for training classes
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "../../src/train/trainer.h"
#include "../../src/train/training_config.h"
#include "../../src/train/gradient/rate_gd_trainer.h"
#include "../../src/arch/output_detection.h"

namespace py = pybind11;

void bind_training(py::module &m) {
    // GradConfig - gradient descent optimizer configuration
    py::class_<GradConfig>(m, "GradConfig",
        "Gradient descent optimizer configuration")
        .def(py::init<>())
        .def_readwrite("loss", &GradConfig::loss,
                      "Loss function: 'softmax_xent'")
        .def_readwrite("temperature", &GradConfig::temperature,
                      "Temperature for softmax")
        .def_readwrite("optimizer", &GradConfig::optimizer,
                      "Optimizer: 'sgd', 'adam', or 'adamw'")
        .def_readwrite("momentum", &GradConfig::momentum,
                      "SGD momentum")
        .def_readwrite("adam_beta1", &GradConfig::adam_beta1,
                      "Adam beta1 parameter")
        .def_readwrite("adam_beta2", &GradConfig::adam_beta2,
                      "Adam beta2 parameter")
        .def_readwrite("adam_eps", &GradConfig::adam_eps,
                      "Adam epsilon for numerical stability")
        .def_readwrite("clip_grad_norm", &GradConfig::clip_grad_norm,
                      "Gradient clipping norm (0 = disabled)")
        .def("__repr__", [](const GradConfig &c) {
            return "<GradConfig optimizer='" + c.optimizer + "'>";
        });
    
    // OutputDetectorConfig - output detection configuration
    py::class_<OutputDetectorConfig>(m, "OutputDetectorConfig",
        "Output detector configuration")
        .def(py::init<>())
        .def_readwrite("type", &OutputDetectorConfig::type,
                      "Detector type: 'ema'")
        .def_readwrite("alpha", &OutputDetectorConfig::alpha,
                      "EMA smoothing factor")
        .def_readwrite("threshold", &OutputDetectorConfig::threshold,
                      "Minimum activity threshold")
        .def_readwrite("default_id", &OutputDetectorConfig::default_id,
                      "Default output when abstaining")
        .def("__repr__", [](const OutputDetectorConfig &c) {
            return "<OutputDetectorConfig type='" + c.type + 
                   "' alpha=" + std::to_string(c.alpha) + ">";
        });
    
    // EpisodeMetrics struct
    py::class_<EpisodeMetrics>(m, "EpisodeMetrics",
        "Metrics from a single episode evaluation")
        .def(py::init<>())
        .def_readwrite("winner_id", &EpisodeMetrics::winner_id,
                      "ID of winning output neuron")
        .def_readwrite("margin", &EpisodeMetrics::margin,
                      "Separation margin between winner and runner-up")
        .def_readwrite("rates", &EpisodeMetrics::rates,
                      "Firing rates for output neurons")
        .def_readwrite("ticks_run", &EpisodeMetrics::ticks_run,
                      "Number of timesteps executed")
        .def("__repr__", [](const EpisodeMetrics &m) {
            return "<EpisodeMetrics winner='" + m.winner_id + 
                   "' margin=" + std::to_string(m.margin) + ">";
        });
    
    // TrainingConfig - comprehensive configuration
    py::class_<TrainingConfig>(m, "TrainingConfig",
        "Training configuration parameters")
        .def(py::init<>())
        
        // Episode timing
        .def_readwrite("warmup_ticks", &TrainingConfig::warmup_ticks)
        .def_readwrite("decision_window", &TrainingConfig::decision_window)
        
        // Learning parameters
        .def_readwrite("lr", &TrainingConfig::lr, "Learning rate")
        .def_readwrite("batch_size", &TrainingConfig::batch_size)
        .def_readwrite("shuffle", &TrainingConfig::shuffle)
        .def_readwrite("weight_decay", &TrainingConfig::weight_decay)
        .def_readwrite("weight_clip", &TrainingConfig::weight_clip)
        
        // Eligibility trace
        .def_readwrite("elig_lambda", &TrainingConfig::elig_lambda)
        .def_readwrite("elig_post_use_rate", &TrainingConfig::elig_post_use_rate)
        
        // Reward
        .def_readwrite("reward_mode", &TrainingConfig::reward_mode)
        .def_readwrite("reward_pos", &TrainingConfig::reward_pos)
        .def_readwrite("reward_neg", &TrainingConfig::reward_neg)
        .def_readwrite("reward_gain", &TrainingConfig::reward_gain)
        .def_readwrite("reward_min", &TrainingConfig::reward_min)
        .def_readwrite("reward_max", &TrainingConfig::reward_max)
        .def_readwrite("margin_delta", &TrainingConfig::margin_delta)
        .def_readwrite("no_update_if_satisfied", &TrainingConfig::no_update_if_satisfied)
        .def_readwrite("use_advantage_baseline", &TrainingConfig::use_advantage_baseline)
        .def_readwrite("baseline_beta", &TrainingConfig::baseline_beta)
        .def_readwrite("update_gating", &TrainingConfig::update_gating)
        
        // Intrinsic plasticity
        .def_readwrite("eta_theta", &TrainingConfig::eta_theta)
        .def_readwrite("eta_leak", &TrainingConfig::eta_leak)
        .def_readwrite("r_target", &TrainingConfig::r_target)
        .def_readwrite("rate_alpha", &TrainingConfig::rate_alpha)
        
        // Structural plasticity
        .def_readwrite("prune_epsilon", &TrainingConfig::prune_epsilon)
        .def_readwrite("prune_patience", &TrainingConfig::prune_patience)
        .def_readwrite("grow_edges", &TrainingConfig::grow_edges)
        .def_readwrite("init_weight", &TrainingConfig::init_weight)
        .def_readwrite("usage_boost_gain", &TrainingConfig::usage_boost_gain)
        .def_readwrite("inactive_rate_threshold", &TrainingConfig::inactive_rate_threshold)
        .def_readwrite("inactive_rate_patience", &TrainingConfig::inactive_rate_patience)
        .def_readwrite("prune_inactive_max", &TrainingConfig::prune_inactive_max)
        .def_readwrite("prune_inactive_out", &TrainingConfig::prune_inactive_out)
        .def_readwrite("prune_inactive_in", &TrainingConfig::prune_inactive_in)
        
        // Logging and reproducibility
        .def_readwrite("verbose", &TrainingConfig::verbose)
        .def_readwrite("log_every", &TrainingConfig::log_every)
        .def_readwrite("seed", &TrainingConfig::seed)
        .def_readwrite("weight_jitter_std", &TrainingConfig::weight_jitter_std)
        
        // Checkpointing
        .def_readwrite("checkpoints_enable", &TrainingConfig::checkpoints_enable)
        .def_readwrite("ckpt_l0", &TrainingConfig::ckpt_l0)
        .def_readwrite("ckpt_l1", &TrainingConfig::ckpt_l1)
        .def_readwrite("ckpt_l2", &TrainingConfig::ckpt_l2)
        
        // Revert/rollback
        .def_readwrite("revert_enable", &TrainingConfig::revert_enable)
        .def_readwrite("revert_metric", &TrainingConfig::revert_metric)
        .def_readwrite("revert_window", &TrainingConfig::revert_window)
        .def_readwrite("revert_drop", &TrainingConfig::revert_drop)
        
        // Nested configs
        .def_readwrite("grad", &TrainingConfig::grad,
                      "Gradient descent optimizer configuration")
        .def_readwrite("detector", &TrainingConfig::detector,
                      "Output detector configuration")
        
        .def("__repr__", [](const TrainingConfig &c) {
            return "<TrainingConfig lr=" + std::to_string(c.lr) +
                   " batch=" + std::to_string(c.batch_size) + 
                   " optimizer='" + c.grad.optimizer + "'>";
        });
    
    // EpisodeData struct
    py::class_<Trainer::EpisodeData>(m, "EpisodeData",
        "Episode data: input sequence + target output")
        .def(py::init<>())
        .def_readwrite("seq", &Trainer::EpisodeData::seq)
        .def_readwrite("target_id", &Trainer::EpisodeData::target_id);
    
    // Trainer class
    py::class_<Trainer, std::shared_ptr<Trainer>>(m, "Trainer",
        "Neural network trainer with gradient-based methods\n\n"
        "Example:\n"
        "    >>> trainer = Trainer(network)\n"
        "    >>> trainer.train_epoch(dataset, epochs=100, config=cfg)\n")
        
        .def(py::init<Glia&>(),
             py::arg("network"),
             "Create trainer for a network")
        
        .def("reseed", &Trainer::reseed,
             py::arg("seed"),
             "Set random seed for reproducibility")
        
        .def("evaluate", &Trainer::evaluate,
             py::arg("sequence"), py::arg("config"),
             py::call_guard<py::gil_scoped_release>(),
             "Evaluate network on single episode (GIL released)")
        
        .def("train_batch", [](Trainer &self, 
                                const std::vector<Trainer::EpisodeData> &batch,
                                const TrainingConfig &config) {
            py::gil_scoped_release release;
            self.trainBatch(batch, config, nullptr);
        },
        py::arg("batch"), py::arg("config"),
        "Train on a batch of episodes (GIL released)")
        
        .def("train_epoch", &Trainer::trainEpoch,
             py::arg("dataset"), py::arg("epochs"), py::arg("config"),
             py::call_guard<py::gil_scoped_release>(),
             "Train for multiple epochs (GIL released)\n\n"
             "Note: This releases the GIL for the duration of training.\n"
             "For Python callbacks, use the Python wrapper in glia.trainer")
        
        .def("get_epoch_acc_history", &Trainer::getEpochAccHistory,
             "Get accuracy history over epochs")
        
        .def("get_epoch_margin_history", &Trainer::getEpochMarginHistory,
             "Get margin history over epochs")
        
        .def("revert_checkpoint", &Trainer::revertCheckpoint,
             "Revert to last checkpoint (if checkpointing enabled)")
        
        .def("__repr__", [](const Trainer &t) {
            return "<Trainer>";
        });
    
    // Output detector configuration
    py::class_<OutputDetectorOptions>(m, "OutputDetectorOptions")
        .def(py::init<>())
        .def_readwrite("threshold", &OutputDetectorOptions::threshold)
        .def_readwrite("default_id", &OutputDetectorOptions::default_id);
    
    // RateGDTrainer class - Gradient-based trainer (proper supervised learning)
    py::class_<RateGDTrainer, std::shared_ptr<RateGDTrainer>>(m, "RateGDTrainer",
        "Rate-based gradient descent trainer for supervised learning\n\n"
        "Example:\n"
        "    >>> trainer = RateGDTrainer(network)\n"
        "    >>> trainer.train_epoch(dataset, epochs=100, config=cfg)\n")
        
        .def(py::init<Glia&>(),
             py::arg("network"),
             "Create gradient trainer for a network")
        
        .def("reseed", &RateGDTrainer::reseed,
             py::arg("seed"),
             "Set random seed for reproducibility")
        
        .def("evaluate", &RateGDTrainer::evaluate,
             py::arg("sequence"), py::arg("config"),
             py::call_guard<py::gil_scoped_release>(),
             "Evaluate network on single episode (GIL released)")
        
        .def("train_batch", [](RateGDTrainer &self, 
                                const std::vector<Trainer::EpisodeData> &batch,
                                const TrainingConfig &config) {
            py::gil_scoped_release release;
            self.trainBatch(batch, config, nullptr);
        },
        py::arg("batch"), py::arg("config"),
        "Train on a batch of episodes (GIL released)")
        
        .def("train_epoch", &RateGDTrainer::trainEpoch,
             py::arg("dataset"), py::arg("epochs"), py::arg("config"),
             py::call_guard<py::gil_scoped_release>(),
             "Train for multiple epochs (GIL released)")
        
        .def("get_epoch_acc_history", &RateGDTrainer::getEpochAccHistory,
             "Get accuracy history over epochs")
        
        .def("get_epoch_margin_history", &RateGDTrainer::getEpochMarginHistory,
             "Get margin history over epochs")
        
        .def("__repr__", [](const RateGDTrainer &t) {
            return "<RateGDTrainer (gradient-based)>";
        });
}
