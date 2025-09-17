// training_glue.cpp
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "glia.h"
#include "neuron.h"
#include "trainer.hpp"

using namespace glia_training;

class TrainerGlue {
public:
    explicit TrainerGlue(Glia& glia, const TrainerConfig& cfg = TrainerConfig())
        : glia_(glia) {
        rebuild_index();
        rebuild_edges();
        build_io();
        trainer_ = std::make_unique<Trainer>(cfg, io_);
    }

    void on_step_begin() { rebuild_edges(); trainer_->on_step_begin(); }
    void on_step_end()   { trainer_->on_step_end(); }

    void inject_by_id(const std::string& id, float amt) {
        auto it = id2idx_.find(id); if (it==id2idx_.end()) return; inject_by_index(it->second, amt);
    }
    void inject_by_index(int idx, float amt) {
        if (idx<0 || idx>=(int)idx2ptr_.size()) return; idx2ptr_[idx]->receive(amt);
    }

    void rebuild_index() {
        idx2ptr_.clear(); id2idx_.clear();
        glia_.forEachNeuron([&](Neuron& n){
            id2idx_[n.getId()] = (int)idx2ptr_.size();
            idx2ptr_.push_back(&n);
        });
        edge_view_.assign(idx2ptr_.size(), {});
    }

    const std::vector<std::unordered_map<int,float>>& edge_view() const { return edge_view_; }

private:
    Glia& glia_;
    std::vector<Neuron*> idx2ptr_;
    std::unordered_map<std::string,int> id2idx_;
    std::vector<std::unordered_map<int,float>> edge_view_;
    NetworkIO io_{};
    std::unique_ptr<Trainer> trainer_;

    void rebuild_edges() {
        for (int i=0;i<(int)idx2ptr_.size();++i) {
            edge_view_[i].clear();
            const auto& conns = idx2ptr_[i]->getConnections();
            for (const auto& kv : conns) {
                auto it = id2idx_.find(kv.first);
                if (it != id2idx_.end()) edge_view_[i][it->second] = kv.second.first; // weight
            }
        }
    }
    void build_io() {
        io_.num_neurons = [&]{ return idx2ptr_.size(); };
        io_.fired       = [&](size_t i){ return idx2ptr_[i]->didFire(); };
        io_.out_edges   = [&](size_t i) -> const std::unordered_map<int,float>& { return edge_view_[i]; };
        io_.set_weight  = [&](size_t i,int j,float w){ idx2ptr_[i]->setTransmitter(idx2ptr_[j]->getId(), w); };
        io_.remove_edge = [&](size_t i,int j){ idx2ptr_[i]->removeConnection(idx2ptr_[j]->getId()); };
        io_.add_edge    = [&](size_t i,int j,float w){ idx2ptr_[i]->addConnection(w, *idx2ptr_[j]); };
    }
};