#ifndef _TOPOLOGY_POLICY_H_
#define _TOPOLOGY_POLICY_H_

#include <string>

// TopologyPolicy defines structural constraints for networks.
// Enforcement is intended for training/build-time; runtime sim does not enforce.
struct TopologyPolicy {
    // Disallow any inbound edges to sensory neurons (S* are pure sources)
    bool allow_inbound_to_sensory = false;

    // Disallow feedback into outputs (O* act as sinks by default)
    bool allow_feedback_to_outputs = false;

    // Optional extras for future use
    bool allow_self_loops = false;

    // Check if an edge from `from_id` -> `to_id` is permitted by policy
    bool edgeAllowed(const std::string &from_id, const std::string &to_id) const {
        if (!allow_inbound_to_sensory && !to_id.empty() && to_id[0] == 'S') {
            return false; // No inbound to S*
        }
        if (!allow_feedback_to_outputs && !to_id.empty() && to_id[0] == 'O') {
            // Block O* as targets (i.e., no inbound to O*) when feedback is off
            return false;
        }
        if (!allow_self_loops && from_id == to_id) {
            return false;
        }
        return true;
    }
};

#endif // _TOPOLOGY_POLICY_H_
