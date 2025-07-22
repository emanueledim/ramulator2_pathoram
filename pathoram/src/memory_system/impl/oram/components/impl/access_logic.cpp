#include "memory_system/impl/oram/components/inc/access_logic.h"

namespace Ramulator {

AccessLogic::AccessLogic(Addr_t max_paddr, int block_size, int z_blocks, int arity, OOBTree* oob_tree) :
    max_paddr(max_paddr), block_size(block_size), z_blocks(z_blocks), arity(arity) {
    this->oob_tree = oob_tree;

    bucket_size = block_size * z_blocks;
    base_address_headers_tree = max_paddr/(z_blocks + 1.0) * z_blocks;
    int num_buckets = (z_blocks/(z_blocks + 1.0) * max_paddr) / bucket_size;
    tree_depth = static_cast<int>(std::log2(num_buckets + 1) - 1);

    std::random_device rd;
    rng.seed(rd());
    leaf_dist = std::uniform_int_distribution<int>(0, (int)pow(arity, tree_depth)-1);
}

int AccessLogic::get_random_leaf() {
    return leaf_dist(rng);
}

int AccessLogic::get_tree_depth() {
    return tree_depth;
}

int AccessLogic::get_bucket_index(Addr_t addr) {
    return addr / bucket_size;
}
        
int AccessLogic::get_block_offset(Addr_t addr) {
    int block_offset = addr % (bucket_size);
    return block_offset / block_size;
}

Addr_t AccessLogic::generate_next_hdr_address(int leaf) {
    std::list<Addr_t> addrs = access_headers_path(leaf);
    auto it = addrs.begin();
    if(cnt_addr >= addrs.size()) {
        cnt_addr = 0;
        return -1;
    }
    std::advance(it, cnt_addr);
    if(it == addrs.end()) return -1;
    cnt_addr++;
    return *it;
}

Addr_t AccessLogic::generate_next_data_address(int leaf) {
    std::list<Addr_t> addrs = access_data_path(leaf);
    auto it = addrs.begin();
    if(cnt_addr >= addrs.size()) {
        cnt_addr = 0;
        return -1;
    }
    std::advance(it, cnt_addr);
    if(it == addrs.end()) return -1;
    cnt_addr++;
    return *it;
}

std::list<int> AccessLogic::get_path_indexes(int leaf) {
    std::list<int> indexes;
    int base_leaf = pow(arity, tree_depth);
    int index_node = leaf + base_leaf;
    while(index_node > 0) {
        indexes.push_front(index_node-1);
        index_node = index_node / arity;
    }
    return indexes;
}

void AccessLogic::init_path(int leaf) {
    std::list<int> indexes = get_path_indexes(leaf);
    for(auto bucket_idx : indexes) {
        oob_tree->insert_bucket(bucket_idx, Bucket(z_blocks));
    }
}

std::list<Addr_t> AccessLogic::access_data_path(int leaf) {
    std::list<int> path_indexes;
    std::list<Addr_t> path_addresses;
    path_indexes = get_path_indexes(leaf);

    for(auto idx : path_indexes) {
        Addr_t base_bucket_address = idx * bucket_size;
        for(int i=0; i<z_blocks; i++) {
            path_addresses.push_back(base_bucket_address + i * block_size);
        }
    }
    return path_addresses;
}

std::list<Addr_t> AccessLogic::access_headers_path(int leaf) {
    std::list<int> path_indexes;
    std::list<Addr_t> path_addresses;
    path_indexes = get_path_indexes(leaf);

    for(auto idx : path_indexes) {
        Addr_t bucket_header_address = base_address_headers_tree + (idx) * block_size;
        path_addresses.push_back(bucket_header_address);
    }
    return path_addresses;
}

bool AccessLogic::insert_block_random_pos(Addr_t program_addr, int leaf) {
    std::list<int> bucket_indexes = get_path_indexes(leaf);
    int path_size = bucket_indexes.size();
    std::uniform_int_distribution<int> path_dist = std::uniform_int_distribution<int>(0, path_size-1);
    for(int j=0; j<100; j++){
        int random_offset = path_dist(rng);
        auto it = bucket_indexes.begin();
        std::advance(it, random_offset);
        int chosen_bucket_idx = *it;
        for (int i = 0; i < z_blocks; ++i) {
            if (oob_tree->is_dummy(chosen_bucket_idx, i)) {
                oob_tree->insert_block_header(chosen_bucket_idx, i, BlockHeader(program_addr, leaf));
                return true;
            }
        }
    }
    return false;
}

bool AccessLogic::is_common_bucket(int leaf1, int leaf2, int level) {
    std::list<int> path1 = get_path_indexes(leaf1);
    std::list<int> path2 = get_path_indexes(leaf2);
    path1.reverse();
    path2.reverse();

    auto it1 = path1.begin();
    auto it2 = path2.begin();
    
    std::advance(it1, level);
    std::advance(it2, level);

    if( (*it1) == (*it2) ) {
        return true;
    }
    return false;
}

Addr_t AccessLogic::writeback_level(int leaf, int level, Addr_t program_addr) {
    int wb_addr = -1;
    std::list<int> path = get_path_indexes(leaf);
    path.reverse();
    auto it = path.begin();
    std::advance(it, level);

    for(int i=0; i<z_blocks; i++) {
        if(oob_tree->is_dummy(*it, i)) {
            wb_addr = (*it) * bucket_size + i * block_size;
            oob_tree->insert_block_header(*it, i, BlockHeader(program_addr, leaf));
            break;
        }
    }
    return wb_addr;
}

}