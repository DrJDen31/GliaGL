#include <string>
#include "neuron.h"

#include <iostream>
#include <fstream>

/*
Constructor

PARAMS:
    id: unique ID string for this cell
    complexity: number of cells in the whole circuit
    resting: resting voltage of the cell
    balancer: voltage correction toward resting each tick
    refractory: how many tick cycles the cell is inactive for after firing
    threshold: voltage at which the cell fires
    tick: states whether this cell should only fire on tick
*/
Neuron::Neuron(const std::string id,
               const int complexity,
               const float resting,
               const float balancer = 1,
               const int refractory = 0,
               const float threshold = 0,
               const bool tick = true)
{
    this->id = id;
    this->value = resting;
    this->resting = resting;
    this->balancer = balancer;
    this->delta = 0;
    this->on_deck = 0;
    this->refractory = 0;
    this->refractory_period = refractory;
    this->threshold = threshold;
    this->complexity = complexity;
    this->using_tick = tick;
}

/*
Destructor
*/
Neuron::~Neuron()
{
}

/*
Updates the weight of the transmission for a given cell connection

PARAMS:
    id: ID string of the receiving cell of the connection to modify
    new_transmitter: new value to send when this cell fires
*/
void Neuron::setTransmitter(std::string id, float new_transmitter)
{
    this->connections[id].first = new_transmitter;
}

/*
Updates the voltage at which to fire the cell

PARAMS:
    new_threshold: the new voltage value
*/
void Neuron::setThreshold(float new_threshold)
{
    this->threshold = new_threshold;
}

/*
Updates the leak/balancer parameter

PARAMS:
    new_leak: the new leak value
*/
void Neuron::setLeak(float new_leak)
{
    this->balancer = new_leak;
}

/*
Updates the resting voltage

PARAMS:
    new_resting: the new resting voltage value
*/
void Neuron::setResting(float new_resting)
{
    this->resting = new_resting;
    // Also reset current voltage to new resting to avoid transients
    this->value = new_resting;
}

/*
Adds a connection from the axon of this cell to the dendrite of another

PARAMS:
    transmitter: value to send when this cell fires
    neuron: address of the receiving cell
*/
void Neuron::addConnection(float transmitter, Neuron &neuron)
{
    this->connections[neuron.id] = std::make_pair(transmitter, &neuron);
}

/*
Receive a voltage pulse from another cell

PARAMS:
    transmission: voltage being sent
*/
void Neuron::receive(float transmission)
{
    // std::cout << "received " << transmission << std::endl;
    if (this->using_tick)
    {
        // When using tick-based updates, stage the input for next tick
        // DO NOT apply immediately - this maintains 1-tick synaptic delay
        this->on_deck += transmission;
        return;  // Exit early - tick() will handle the voltage update
    }

    // Non-tick mode: apply voltage immediately
    this->value += transmission;

    // no action if threshold not met
    if (this->value <= this->threshold)
    {
        if (this->refractory > 0)
        {
            this->refractory -= 1;
        }
        return;
    }

    // only fire if not in refractory period
    if (this->refractory == 0)
    {
        this->fire();
    }
    else
    {
        this->refractory -= 1;
    }
}

/*
Tick update, make any actions based on state of the cell
*/
void Neuron::tick()
{
    // track fire status for training
    this->just_fired = false;

    // Stage synaptic input
    float incoming = this->delta;
    this->delta = this->on_deck;
    this->on_deck = 0;

    // if on refractory, decrement period
    if (this->refractory > 0)
    {
        this->refractory -= 1;
        return;
    }

    // Apply membrane update: V = max(0, V*leak + incoming)
    // leak=1.0 means voltage persists, leak=0.0 means coincidence detection (no persistence)
    float leak_factor = this->balancer;
    this->value = leak_factor * this->value + incoming;
    if (this->value < 0) this->value = 0;  // max(0, ...)

    // if voltage threshold met, fire
    if (this->value > this->threshold)
    {
        this->fire();
    }
    // else, move towards resting voltage
    // else if (this->value > this->resting)
    // {
    //     this->value -= balancer;
    // }
    // else if (this->value < this->resting)
    // {
    //     this->value += balancer;
    // }
}

/*
Send a transmission to all connected cells
*/
// void Neuron::fire()
// {
//     // track for training
//     this->just_fired = true;

//     std::cout << id << " firing!" << std::endl;
//     // for each connected cell
//     for (std::map<std::string, std::pair<float, Neuron *>>::iterator itr = connections.begin(); itr != connections.end(); itr++)
//     {
//         // send each connected cell the associated transmission value
//         itr->second.second->receive(itr->second.first);
//     }

//     // activate refractory period to prevent quick succession of firings
//     this->refractory = this->refractory_period;
// }
void Neuron::fire()
{
    this->just_fired = true;

    // SOFT RESET: pull membrane toward resting after a spike
    // (tunable factor; start with subtracting half-threshold)
    // this->value = std::max(this->resting, this->value - 0.5f * this->threshold);

    // test with a full reset
    this->value = this->resting;

    // send transmissions
    for (auto itr = connections.begin(); itr != connections.end(); ++itr)
    {
        itr->second.second->receive(itr->second.first);
    }
    // this->refractory = this->refractory_period;
}