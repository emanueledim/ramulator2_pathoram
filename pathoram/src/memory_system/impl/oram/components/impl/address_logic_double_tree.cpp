#include "memory_system/impl/oram/components/inc/address_logic_double_tree.h"

namespace Ramulator {

std::list<Addr_t> AddressLogicDoubleTree::access_data_path(int leaf) {
    std::list<int> path_indexes;
    std::list<Addr_t> path_addresses;
    path_indexes = get_path_indexes(leaf);

    for(auto idx : path_indexes) {
        Addr_t base_bucket_address = oram_tree_info->base_address_tree + (idx * oram_tree_info->bucket_size);
        for(int i = 0; i < oram_tree_info->z_blocks; i++) {
            path_addresses.push_back(base_bucket_address + i * oram_tree_info->block_size);
        }
    }
    return path_addresses;
}

std::list<Addr_t> AddressLogicDoubleTree::access_headers_path(int leaf) {
    std::list<int> path_indexes;
    std::list<Addr_t> path_addresses;
    path_indexes = get_path_indexes(leaf);

    for(auto idx : path_indexes) {
        Addr_t bucket_header_address = base_address_headers_tree + (idx * oram_tree_info->block_size);
        path_addresses.push_back(bucket_header_address);
    }
    return path_addresses;
}

std::list<int> AddressLogicDoubleTree::get_path_indexes(int leaf) {
    std::list<int> indexes;
    int arity = oram_tree_info->arity;
    int tree_depth = oram_tree_info->tree_depth;
    int base_leaf = pow(arity, tree_depth);
    int index_node = leaf + base_leaf;
    while(index_node > 0) {
        indexes.push_front(index_node-1);
        index_node = index_node / arity;
    }
    return indexes;
}

AddressLogicDoubleTree::AddressLogicDoubleTree(OOBTree* oob_tree) {
    this->oob_tree = oob_tree;
    std::random_device rd;
    rng.seed(rd());
}

Addr_t AddressLogicDoubleTree::generate_next_hdr_address(int leaf) {
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

Addr_t AddressLogicDoubleTree::generate_next_address(int leaf) {
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

void AddressLogicDoubleTree::init_path(int leaf) {
    std::list<int> indexes = get_path_indexes(leaf);
    for(auto bucket_idx : indexes) {
        oob_tree->insert_bucket(bucket_idx, Bucket(oram_tree_info->z_blocks));
    }
}

bool AddressLogicDoubleTree::init_block(Addr_t block_id, int leaf) {
    std::list<int> bucket_indexes = get_path_indexes(leaf);
    int path_size = bucket_indexes.size();
    std::uniform_int_distribution<int> path_dist = std::uniform_int_distribution<int>(0, path_size-1);
    for(int j=0; j<100; j++){
        int random_offset = path_dist(rng);
        auto it = bucket_indexes.begin();
        std::advance(it, random_offset);
        int chosen_bucket_idx = *it;
        for (int i = 0; i < oram_tree_info->z_blocks; i++) {
            if (oob_tree->is_dummy(chosen_bucket_idx, i)) {
                oob_tree->insert_block_header(chosen_bucket_idx, i, BlockHeader(block_id, leaf));
                return true;
            }
        }
    }
    return false;
}

bool AddressLogicDoubleTree::is_common_bucket(int leaf1, int leaf2, int level) {
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

Addr_t AddressLogicDoubleTree::writeback_data(int leaf, int level, Addr_t block_id) {
    int wb_addr = -1;
    std::list<int> path = get_path_indexes(leaf);
    auto it = path.begin();
    std::advance(it, level);
    int z_blocks = oram_tree_info->z_blocks;
    for(int i = 0; i < z_blocks; i++) {
        if(oob_tree->is_dummy(*it, i)) {
            wb_addr = oram_tree_info->base_address_tree + ((*it) * oram_tree_info->bucket_size + i * oram_tree_info->block_size);
            oob_tree->insert_block_header(*it, i, BlockHeader(block_id, leaf));
            break;
        }
    }
    return wb_addr;
}

Addr_t AddressLogicDoubleTree::writeback_dummy(int leaf, int level) {
    int wb_addr = -1;
    std::list<int> path = get_path_indexes(leaf);
    auto it = path.begin();
    std::advance(it, level);
    int z_blocks = oram_tree_info->z_blocks;
    if(dummy_wb == z_blocks) {
        dummy_wb = 0;
        return wb_addr;
    }
    while(dummy_wb < z_blocks) {
        if(oob_tree->is_dummy(*it, dummy_wb)) {
            wb_addr = oram_tree_info->base_address_tree + ((*it) * oram_tree_info->bucket_size + dummy_wb * oram_tree_info->block_size);
            dummy_wb++;
            break;
        } else {
            dummy_wb++;
        }
    }
    return wb_addr;

}

void AddressLogicDoubleTree::attach_oram_info(const ORAMTreeInfo* oram_tree_info) {
    this->oram_tree_info = oram_tree_info;
    int length_tree = oram_tree_info->length_tree;
    int base_address_tree = oram_tree_info->base_address_tree;
    int z_blocks = oram_tree_info->z_blocks;
    base_address_headers_tree = (length_tree - base_address_tree)/(z_blocks + 1.0) * z_blocks;
}

}