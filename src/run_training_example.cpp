// This is a *demo harness* that mocks the minimal Glia/Neuron surface to prove
// the trainer works. Replace the mock with your real classes by binding the
// callbacks to your actual data structures.

#include <iostream>
#include <unordered_map>
#include <vector>
#include <random>
#include <algorithm>
#include <cassert>

#include "trainer.hpp"
#include "neuron.h"

struct MockNet
{
    struct Node
    {
        bool fire = false;                  // all-or-nothing spike this step
        float membrane = 0.0f;              // internal potential (simple integrator)
        std::unordered_map<int, float> out; // sparse outgoing edges
    };

    std::vector<Node> nodes;
    float leak = 0.95f;     // membrane leak per step
    float threshold = 1.0f; // fire threshold

    explicit MockNet(size_t n) : nodes(n) {}

    void inject_current(int i, float x) { nodes[i].membrane += x; }

    void step()
    {
        // integrate inputs from last step's spikes
        std::vector<float> incoming(nodes.size(), 0.0f);
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (!nodes[i].fire)
                continue;
            for (auto &kv : nodes[i].out)
            {
                int j = kv.first;
                float w = kv.second;
                incoming[j] += w; // instant transmission for demo
            }
        }
        // update membranes and decide spikes
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            nodes[i].membrane = nodes[i].membrane * leak + incoming[i];
            bool fired = nodes[i].membrane >= threshold;
            nodes[i].fire = fired;
            if (fired)
                nodes[i].membrane = 0.0f; // reset on spike
        }
    }
};

int main()
{
    using namespace glia_training;

    const int N = 8;
    const std::vector<int> inputs = {0, 1};
    const int output = 7;

    MockNet net(N);
    // Initialize a sparse random graph
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> pick(0, N - 1);
    for (int e = 0; e < 12; ++e)
    {
        int i = pick(rng), j = pick(rng);
        if (i == j)
            continue;
        net.nodes[i].out[j] = 0.1f;
    }

    // Wire output to a few nodes so it can learn to respond
    net.nodes[2].out[7] = 0.1f;
    net.nodes[3].out[7] = 0.1f;

    // // Build index ↔ pointer and id ↔ index
    // std::vector<Neuron *> idx2ptr;
    // std::unordered_map<std::string, int> id2idx;
    // glia.forEachNeuron([&](Neuron &n)
    //                    {
    // id2idx[n.getId()] = (int)idx2ptr.size();
    // idx2ptr.push_back(&n); });

    // // Edge views rebuilt each step (so out_edges can return a const ref)
    // std::vector<std::unordered_map<int, float>> edge_view(idx2ptr.size());

    // auto rebuild_edges = [&]
    // {
    //     for (int i = 0; i < (int)idx2ptr.size(); ++i)
    //     {
    //         edge_view[i].clear();
    //         for (auto &kv : idx2ptr[i]->getConnections())
    //         {
    //             auto it = id2idx.find(kv.first);
    //             if (it != id2idx.end())
    //                 edge_view[i][it->second] = kv.second.first; // weight
    //         }
    //     }
    // };

    // NetworkIO io;
    // io.num_neurons = [&]
    // { return idx2ptr.size(); };
    // io.fired = [&](size_t i)
    // { return idx2ptr[i]->didFire(); };
    // io.out_edges = [&](size_t i) -> const std::unordered_map<int, float> &
    // { return edge_view[i]; };
    // io.set_weight = [&](size_t i, int j, float w)
    // {
    //     idx2ptr[i]->setTransmitter(idx2ptr[j]->getId(), w);
    // };
    // io.remove_edge = [&](size_t i, int j)
    // {
    //     idx2ptr[i]->removeConnection(idx2ptr[j]->getId());
    // };
    // io.add_edge = [&](size_t i, int j, float w)
    // {
    //     idx2ptr[i]->addConnection(w, *idx2ptr[j]);
    // };

    // Build NetworkIO callbacks over MockNet
    NetworkIO io;
    io.num_neurons = [&]
    { return net.nodes.size(); };
    io.fired = [&](size_t i)
    { return net.nodes[i].fire; };
    io.out_edges = [&](size_t i) -> const std::unordered_map<int, float> &
    { return net.nodes[i].out; };
    io.set_weight = [&](size_t i, int j, float w)
    { net.nodes[i].out[j] = w; };
    io.remove_edge = [&](size_t i, int j)
    { net.nodes[i].out.erase(j); };
    io.add_edge = [&](size_t i, int j, float w)
    { net.nodes[i].out.emplace(j, w); };

    TrainerConfig cfg;
    cfg.homeo_target_sum = 1.5f; // a bit more mass
    cfg.rewire_prob = 0.05f;
    cfg.max_added_per_step = 3;
    cfg.prune_threshold = 0.01f;
    cfg.lr_hebb = 0.03f;
    cfg.lr_anti = 0.01f;

    Trainer trainer(cfg, io);

    // Define a simple temporal XOR-like pattern across inputs 0 and 1
    // Steps: [ [0], [1], [0,1], [] ] repeated
    PulsePattern pat;
    pat.pulses_per_step = {{0}, {1}, {0, 1}, {}};
    PatternFeeder feeder(inputs, pat, [&](int id, float x)
                         { net.inject_current(id, x); });

    // Run
    for (int t = 0; t < 1000; ++t)
    {
        trainer.on_step_begin(); // update traces
        feeder.feed_step(t);     // drive inputs
        net.step();              // integrate & spike
        trainer.on_step_end();   // learn + rewire

        // crude supervised nudge: encourage output spike on XOR step (exactly one input)
        bool xor_target = ((t % 4) == 0 || (t % 4) == 1);
        if (xor_target && !net.nodes[output].fire)
        {
            // small extra push into output
            net.inject_current(output, 0.5f);
        }

        if (t % 50 == 0)
        {
            std::cout << "t=" << t << " out=" << net.nodes[output].fire
                      << " edges_out(out)=" << net.nodes[output].out.size() << "\n";
        }
    }

    // Print some learned weights into output neuron
    std::cout << "\nLearned incoming to output neuron (id=" << output << "):\n";
    for (int i = 0; i < N; ++i)
    {
        auto it = net.nodes[i].out.find(output);
        if (it != net.nodes[i].out.end())
        {
            std::cout << i << " -> " << output << " : " << it->second << "\n";
        }
    }
    return 0;
}