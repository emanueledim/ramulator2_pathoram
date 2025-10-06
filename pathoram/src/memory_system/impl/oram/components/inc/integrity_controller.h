#ifndef INTEGRITY_CONTROLLER_H
#define INTEGRITY_CONTROLLER_H

#include <vector>
#include <queue>

#include "base/base.h"
#include "base/clocked.h"

#include "memory_system/impl/oram/oob/bucket.h"
#include "memory_system/impl/oram/components/inc/oram_tree_info.h"
#include "memory_system/impl/oram/components/interfaces/iintegrity_controller.h"
#include "memory_system/impl/oram/components/interfaces/ioram_controller.h"

namespace Ramulator {

/**
* @class Integrity Controller
* @brief A simple Integrity Checker component to model the delay of the hash calculation.
* It cointains a queue where blocks are enqueued and serialized based on
* their level tree and processed to verify the hash.
*/
class IntegrityController : public IIntegrityController, Clocked<IntegrityController> {
    
        enum class State {Init, Idle, Serialize, CheckIntegrity, SendSignal};

        struct IntegrityEntry {
            bool full;
            std::vector<bool> valid_flags;
            Bucket bucket;

            IntegrityEntry() {};
            IntegrityEntry(int z) : full(false), valid_flags(z, false), bucket(z) {}
        };

    private:
        //Current state
        State current_state = State::Init;
        IORAMController* oram_controller;
        const ORAMTreeInfo* oram_tree_info;

        //The delay of calculating an hash 
        int hashing_delay;

        //The future Clock Cycles in which the end of hash calculation will end
        int remaining_hash_tick = 0;

        //Queue of the pending blocks to check
        std::queue<Request> pending_blocks;
        std::vector<IntegrityEntry> serialized_buckets;

        //Counters
        size_t active_cycles = 0;
        size_t idle_cycles = 0;
        size_t num_reqs = 0;
        size_t latency = 0;
        size_t arrival_time = 0;

        void init_entry(int pos);

        void init_serialized_queue();

        void set_valid(int pos, int offset);

        int num_valid();

        void serialize();

        void handle_check_integrity();

    public:
        IntegrityController();

        IntegrityController(int hashing_delay);
        
        void tick() override;
        
        void enqueue_block(Request& req) override;

        void connect_oram_controller(IORAMController* oram_controller) override;

        void attach_oram_info(const ORAMTreeInfo* oram_tree_info) override;
        
        void set_counters(std::map<std::string, size_t&>& counters) override;
};

}

#endif   // INTEGRITY_CONTROLLER_H