#ifndef __neuron_h__
#define __neuron_h__

#include <map>
#include <string>
#include <memory>
/*
Class representing a single neuron cell, each connecting to various other cells that it forwards its message to
*/
class Neuron
{
public:
    // constructors and destructor
    Neuron(const std::string id, const int complexity, const float resting, const float balancer, const int refractory, const float threshold, const bool tick);
    ~Neuron();

    // getters/setters
    float getValue() const { return value; };
    void setTransmitter(std::string id, float new_transmitter);
    float getThreshold() const { return threshold; };
    void setThreshold(float new_threshold);
    float getLeak() const { return balancer; };
    void setLeak(float new_leak);
    float getResting() const { return resting; };
    void setResting(float new_resting);

    // modifiers
    void addConnection(float transmitter, std::shared_ptr<Neuron> neuron);
    void receive(float transmission);
    void tick();

    // training
    const std::string &getId() const { return id; }
    const std::map<std::string, std::pair<float, std::shared_ptr<Neuron>>> &getConnections() const { return connections; }
    void removeConnection(const std::string &to) { connections.erase(to); }
    bool didFire() const { return just_fired; }

private:
    // member variables
    float value;                                                   // represents the current voltage value of neuron
    float resting;                                                 // represents the resting voltage value
    float balancer;                                                // delta voltage towards resting each tick
    float delta;                                                   // sum of changes from connected neurons firing on this one - to be applied this time step
    float on_deck;                                                 // sum of changes from connected neurons firing on this one - to be applied next time step
                                                                   /*
                                                                   // on_deck acts as a buffer to ensure all neurons connected to this one get their changes in
                                                                   // changes are then shifted into delta, then are applied
                                                                   */
    int refractory;                                                // current refractory state of the cell
    int refractory_period;                                         // refractory period after each firing
    float threshold;                                               // voltage threshold at which the cell fires
    int complexity;                                                // represents the complexity of the circuit - how many neurons there are
    bool using_tick;                                               // states whether tick is being used
    std::string id;                                                // unique ID of the cell
    std::map<std::string, std::pair<float, std::shared_ptr<Neuron>>> connections; // map of all cells whose dendrites receive from this cells axon
    // key is ID of the receiving cell, value is a pair containing the transmission and a shared_ptr to the receiving cell

    // training
    bool just_fired = false; // states whether neuron fired this step

    // member functions
    void fire();
};

#endif