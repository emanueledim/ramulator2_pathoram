#include "memory_system/impl/oram/components/inc/position_map.h"

namespace Ramulator {

PositionMap::PositionMap() {}

bool PositionMap::add_entry(Addr_t program_addr, int leaf) {
    bool inserted = position_map.insert({program_addr, leaf}).second;
    if(inserted) {
        num_entries++;
    }
    return inserted;
}

bool PositionMap::remove_entry(Addr_t program_addr) {
    int deleted_entries = position_map.erase(program_addr);
    num_entries -= deleted_entries;
    return deleted_entries > 0;
}

bool PositionMap::remap(Addr_t program_addr, int new_leaf) {
    position_map.at(program_addr) = new_leaf;
    num_remappings++;
    return true;
}

int PositionMap::get_leaf(Addr_t program_addr) {
    return position_map.at(program_addr);
}

bool PositionMap::is_present(Addr_t program_addr) {
    return position_map.find(program_addr) != position_map.end();
}

int PositionMap::get_num_entries() const {
    return num_entries;
}

int PositionMap::get_num_remappings() const {
    return num_remappings;
}

void PositionMap::dump() {
    std::cout << "Position map:" << std::endl;
    for (const auto& pair : position_map) {
        std::cout << "Addr: "<< pair.first << " | Leaf: " << pair.second << std::endl;
    }
}

}