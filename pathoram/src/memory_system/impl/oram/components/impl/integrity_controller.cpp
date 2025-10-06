#include "memory_system/impl/oram/oram_controller.h"
#include "memory_system/impl/oram/components/inc/integrity_controller.h"

namespace Ramulator {

IntegrityController::IntegrityController(): hashing_delay(0) { }

IntegrityController::IntegrityController(int hashing_delay): hashing_delay(hashing_delay) { }

void IntegrityController::init_entry(int pos) {
    serialized_buckets.at(pos) = IntegrityEntry(oram_tree_info->z_blocks);
}

void IntegrityController::init_serialized_queue() {
    serialized_buckets.resize(oram_tree_info->levels);
    for (int i = 0; i < oram_tree_info->levels; i++) {
        init_entry(i);
    }
}

void IntegrityController::set_valid(int pos, int offset) {
    serialized_buckets.at(pos).valid_flags.at(offset) = true;
    bool full = true;
    for(auto v : serialized_buckets.at(pos).valid_flags) {
        if(!v) {
            full = false;
        }
    }
    serialized_buckets.at(pos).full = full;
}

int IntegrityController::num_valid() {
    int n_valid = 0;
    for (auto b : serialized_buckets) {
        if(b.full)
        n_valid++;
    }
    return n_valid;
}

void IntegrityController::serialize() {
    // Check if there are pending blocks
    if (pending_blocks.empty()) return;

    // Select the front block
    Request next_req = pending_blocks.front();

    // Calculate the level of the tree
    Addr_t node_idx = (next_req.addr - oram_tree_info->base_address_tree) / oram_tree_info->bucket_size;
    int node_idx2 = node_idx+1;
    int level = 0;
    int a = calc_log2(oram_tree_info->arity);
    while ((node_idx2 >>= a)) {
        level++;
    }
    
    // Calculate the offset of the block
    int offset = oram_tree_info->get_block_offset(next_req.addr);

    // Set valid
    set_valid(level, offset);

    // Remove the element just processed
    pending_blocks.pop();
}

void IntegrityController::handle_check_integrity() {
    if (remaining_hash_tick > 0) {
        remaining_hash_tick--;
    } else if (remaining_hash_tick <= 0) {
        serialized_buckets.erase(serialized_buckets.end());
        remaining_hash_tick = hashing_delay;
    }
}

void IntegrityController::tick() {
    m_clk++;
    if (current_state == State::SendSignal) {
        active_cycles++;
        Addr_t addr_stub = 0;
        oram_controller->integrity_check(addr_stub);
        init_serialized_queue();
        latency += m_clk - arrival_time;
        current_state = State::Idle;
    } else if (current_state == State::CheckIntegrity) {
        active_cycles++;
        handle_check_integrity();
        if (serialized_buckets.empty()) {
            current_state = State::SendSignal;
        }
    } else if (current_state == State::Serialize) {
        active_cycles++;
        if (num_valid() == oram_tree_info->levels) {
            remaining_hash_tick = hashing_delay;
            arrival_time = m_clk;
            current_state = State::CheckIntegrity;
        } else {
            if (!pending_blocks.empty()) {
                serialize();
            } else {
                current_state = State::Idle;
            }
        }
    } else if (current_state == State::Idle) {
        idle_cycles++;
        if (!pending_blocks.empty()) {
            current_state = State::Serialize;
        }
    } else if (current_state == State::Init) {
        init_serialized_queue();
        current_state = State::Idle;
    }
}

void IntegrityController::enqueue_block(Request& req) {
    num_reqs++;
    if(hashing_delay > 0) {
        pending_blocks.push(req);
    } else {
        oram_controller->integrity_check(req.addr);
    }
}

void IntegrityController::connect_oram_controller(IORAMController* oram_controller) {
    this->oram_controller = oram_controller;
}

void IntegrityController::attach_oram_info(const ORAMTreeInfo* oram_tree_info) {
    this->oram_tree_info = oram_tree_info;
}

void IntegrityController::set_counters(std::map<std::string, size_t&>& counters) {
    counters.insert({"integrity_controller_idle_cycles", idle_cycles});
    counters.insert({"integrity_controller_active_cycles", active_cycles});
    counters.insert({"integrity_controller_num_reqs", num_reqs});
    counters.insert({"integrity_controller_latency", latency});
}

}