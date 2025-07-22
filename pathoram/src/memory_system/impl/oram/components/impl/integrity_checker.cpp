#include "memory_system/impl/oram/components/inc/integrity_checker.h"

namespace Ramulator {

IntegrityChecker::IntegrityChecker() : delay_calculation(10) { }
IntegrityChecker::IntegrityChecker(Clk_t delay_calculation) : delay_calculation(delay_calculation) { }
        
void IntegrityChecker::calculate_hash() {
    if(clk_end_hash > m_clk) {
        clk_end_hash += delay_calculation;
    } else {
        clk_end_hash = m_clk + delay_calculation;
    }
}

bool IntegrityChecker::check_integrity() {
    return true;
}

void IntegrityChecker::tick() {
    m_clk++;
            
    if(pending_blocks.size() < 2) return;

    switch (curr_phase) {
        case Phase::IntegrityCheck:
        // Check integrity; If it fails, send an exception to CPU.
        check_integrity();
        pending_blocks.pop();
        curr_phase = Phase::Idle;
        break;

        case Phase::WaitCalc:
        if(m_clk > clk_end_hash) {
            curr_phase = Phase::IntegrityCheck;
        }
        break;

        case Phase::HashCalc:
        calculate_hash();
        curr_phase = Phase::WaitCalc;
        break;

        case Phase::Idle:
        curr_phase = Phase::HashCalc;
        break;

        default:
        break;
    }            
}
        
void IntegrityChecker::enqueue_block() {
    pending_blocks.push(StubBlock());
}

}