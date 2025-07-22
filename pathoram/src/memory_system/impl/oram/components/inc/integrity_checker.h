#ifndef INTEGRITY_CHECKER_H
#define INTEGRITY_CHECKER_H

#include "base/base.h"
#include "base/clocked.h"

namespace Ramulator {

/**
* @class Integrity Checker
* @brief A simple components to model the delay of the hash calculation.
* It cointains a queue where (stub) blocks are enqueued in order to 
* execute the hash calculation and verify the digests.
*/
class IntegrityChecker : public Clocked<IntegrityChecker> {
    
        enum class Phase {Idle, HashCalc, WaitCalc, IntegrityCheck};
    private:
        // Stubs
        struct StubBlock{};
        char h0;

        //The delay of calculating an hash 
        Clk_t delay_calculation;

        //The future Clock Cycles in which the end of hash calculation will end
        Clk_t clk_end_hash = 0;

        //Queue of the pending blocks to check
        std::queue<StubBlock> pending_blocks;

        //Current phase
        Phase curr_phase;

    public:
        IntegrityChecker();
        IntegrityChecker(Clk_t delay_calculation);
        
        /**
         * @brief Simulates the delay for hashing calculation.
         * It adds the fixed delay for hash calculation to the current clock cycle.
         */
        void calculate_hash();

        /** Assumption for further implementation:
         * every bucket has an header with 2 hashes (left and right child nodes).
         * The hash is stored in the parent's header.
         * calculated_hash provide the child hash node, and it should be used here.
         */
        bool check_integrity();

        void tick();
        
        /**
         * @brief Enqueue a stub block to IntegrityChecker components.
         */
        void enqueue_block();
};

}

#endif   // INTEGRITY_CHECKER_H