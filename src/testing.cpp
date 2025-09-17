#include "glia.h"
#include "neuron.h"
#include <string>

#include "trainer.hpp"
#include "training_glue.cpp" // or build as a separate TU and link

int main()
{
    // testing config (should be read from file)
    int num_sensory = 10;
    int num_neurons = 10;

    // init object
    Glia *glia = new Glia(num_sensory, num_neurons);

    // configuring from file
    std::string filepath = "hi";
    glia->configureNetworkFromFile(filepath);
    glia->printNetwork();

    glia_training::TrainerConfig cfg;
    cfg.rng_seed = 42;       // reproducible runs
    cfg.rewire_prob = 0.05f; // try structural plasticity
    cfg.max_added_per_step = 3;
    cfg.homeo_target_sum = 1.5f;

    TrainerGlue glue(*glia, cfg);

    for (int t = 0; t < 100; ++t)
    {
        glue.on_step_begin(); // updates traces
        // (optional) glue.inject_by_id("S0", 20.f);  // drive inputs
        glia->step();       // your normal tick loop (sensory receive + tick, interneurons tick) :contentReference[oaicite:3]{index=3}
        glue.on_step_end(); // plasticity + sparse rewiring
    }

    glia->printNetwork();
}