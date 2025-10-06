#include "memory_system/impl/oram/components/inc/stash.h"

namespace Ramulator {

Stash::Stash() : max_stash_size(8192), current(stash.end()) {}

Stash::Stash(int max_stash_size) : max_stash_size(max_stash_size), current(stash.end()) {}

bool Stash::add_entry(BlockHeader block_header) {
    if(stash.size() >= max_stash_size) throw "Stash full";
    stash.insert({block_header.block_id, block_header.leaf});
    return true;
}

bool Stash::remove_entry(Addr_t block_id) {
    auto it = stash.find(block_id);
    if (it == stash.end()) return false;

    // If it is deleting the element pointed by current, move it to next element
    if (it == current) {
        current++;
    }
    stash.erase(it);

    // If it deleted the last element, reset the iterator
    if (current == stash.end() && !stash.empty()) {
        current = stash.begin();
    }
    return true;
}

bool Stash::remap(Addr_t block_id, int new_leaf) {
    stash.at(block_id) = new_leaf;
    return true;
}

bool Stash::is_present(Addr_t block_id) {
    return stash.find(block_id) != stash.end();
}

int Stash::get_leaf(Addr_t block_id) {
    return stash.at(block_id);
}

bool Stash::is_empty() {
    return stash.empty();
}

BlockHeader Stash::next() {
    if (stash.empty()) {
        return BlockHeader(-1, -1);
    }
    if(current == stash.end()) {
        current = stash.begin();
        return BlockHeader(-1, -1);
    }
    BlockHeader bh = BlockHeader(current->first, current->second);
    ++current;
    return bh;
};

void Stash::reset() {
    current = stash.begin();
}

float Stash::occupancy() {
    return (stash.size()/((float)max_stash_size)) * 100;
}

void Stash::dump() {
    if(is_empty()) return;
    std::cout << "Stash:" << std::endl;
    for (const auto& pair : stash) {
        std::cout << "Addr: "<< pair.first << " | Leaf: " << pair.second << std::endl;
    }
}

}