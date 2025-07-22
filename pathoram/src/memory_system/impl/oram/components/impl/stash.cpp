#include "memory_system/impl/oram/components/inc/stash.h"

namespace Ramulator {

Stash::Stash() : max_stash_size(8192) {}

Stash::Stash(int max_stash_size) : max_stash_size(max_stash_size) {}

bool Stash::add_entry(BlockHeader block_header) {
    if(stash.size() >= max_stash_size) throw "Stash full";
    stash.insert({block_header.program_addr, block_header.leaf});
    return true;
}

bool Stash::remove_entry(Addr_t program_addr) {
    return stash.erase(program_addr) > 0;
}

bool Stash::remap(Addr_t program_addr, int new_leaf) {
    stash.at(program_addr) = new_leaf;
    return true;
}

bool Stash::is_present(Addr_t program_addr) {
    return stash.find(program_addr) != stash.end();
}

int Stash::get_leaf(Addr_t program_addr) {
    return stash.at(program_addr);
}

bool Stash::is_empty() {
    return stash.empty();
}

std::unordered_map<Addr_t, int>::iterator Stash::end() {
    return stash.end();
}

std::unordered_map<Addr_t, int>::iterator Stash::next() {
    if (stash.empty() || current_index >= stash.size()) {
        last_index = 0;
        current_index = 0;
        return stash.end();
    }
    auto it = stash.begin();
    std::advance(it, current_index);
    last_index = current_index;
    current_index = current_index+1;
    return it;
}

std::unordered_map<Addr_t, int>::iterator Stash::current() {
    if (stash.empty() || current_index == stash.size()) return stash.end();
    auto it = stash.begin();
    std::advance(it, last_index);
    return it;
}

bool Stash::erase() {
    if (stash.empty()) return false;
    auto it = stash.begin();
    std::advance(it, last_index);
    current_index = last_index;
    stash.erase(it);
    return true;
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