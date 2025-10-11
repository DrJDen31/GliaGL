// Deprecated: input_sequence.h moved to src/arch/input_sequence.h.
// Include the new path directly.
#error "Do not include src/vis/input_sequence.h. Use src/arch/input_sequence.h instead."

// =====================================================================================
// InputSequence - Defines a sequence of sensory inputs over time for testing
// =====================================================================================

struct InputEvent {
    int tick;                              // Which tick to apply this input
    std::map<std::string, float> inputs;   // neuron_id -> input_value
    
    InputEvent(int t) : tick(t) {}
};

class InputSequence {
public:
    InputSequence() : current_tick(0), loop(false) {}
    
    // Add an input event at a specific tick
    void addEvent(int tick, const std::string& neuron_id, float value) {
        // Find or create event at this tick
        InputEvent* event = nullptr;
        for (auto& e : events) {
            if (e.tick == tick) {
                event = &e;
                break;
            }
        }
        if (!event) {
            events.push_back(InputEvent(tick));
            event = &events.back();
        }
        event->inputs[neuron_id] = value;
    }
    
    // Get inputs for the current tick
    std::map<std::string, float> getCurrentInputs() const {
        for (const auto& event : events) {
            if (event.tick == current_tick) {
                return event.inputs;
            }
        }
        return std::map<std::string, float>();  // No inputs this tick
    }
    
    // Advance to next tick
    void advance() {
        current_tick++;
        if (loop && current_tick > getMaxTick()) {
            current_tick = 0;  // Loop back to start
        }
    }
    
    // Reset to beginning
    void reset() {
        current_tick = 0;
    }
    
    // Getters
    int getCurrentTick() const { return current_tick; }
    int getMaxTick() const {
        int max_tick = 0;
        for (const auto& event : events) {
            if (event.tick > max_tick) max_tick = event.tick;
        }
        return max_tick;
    }
    bool isLooping() const { return loop; }
    void setLoop(bool l) { loop = l; }
    bool isEmpty() const { return events.empty(); }
    
    // Clear all events
    void clear() {
        events.clear();
        current_tick = 0;
    }
    
    // Load from file
    bool loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open sequence file: " << filepath << std::endl;
            return false;
        }
        
        clear();
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            
            if (cmd == "DURATION") {
                // Just informational, events define actual duration
                continue;
            } else if (cmd == "LOOP") {
                std::string loop_val;
                iss >> loop_val;
                loop = (loop_val == "true" || loop_val == "1");
            } else if (cmd == "EVENT" || std::isdigit(cmd[0])) {
                // Format: TICK NEURON_ID VALUE
                // or: EVENT TICK NEURON_ID VALUE
                int tick;
                std::string neuron_id;
                float value;
                
                if (cmd == "EVENT") {
                    iss >> tick >> neuron_id >> value;
                } else {
                    // First token is tick
                    tick = std::stoi(cmd);
                    iss >> neuron_id >> value;
                }
                
                addEvent(tick, neuron_id, value);
            }
        }
        
        file.close();
        return true;
    }

private:
    std::vector<InputEvent> events;
    int current_tick;
    bool loop;
};

// (Intentionally no include guard; this header should not be used.)
