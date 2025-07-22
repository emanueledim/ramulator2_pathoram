#include "memory_system/impl/oram/oob/oob_tree.h"

namespace Ramulator {

bool OOBTree::insert_bucket(int bucket_index, const Bucket& bucket) {
    return buckets.insert({bucket_index, bucket}).second;
}

bool OOBTree::insert_block_header(int bucket_index, int block_offset, BlockHeader block_header) {
    auto it = buckets.find(bucket_index);
    if (it == buckets.end()) return false;
    it->second.block_headers.at(block_offset).program_addr = block_header.program_addr;
    it->second.block_headers.at(block_offset).leaf = block_header.leaf;
    return true;
}

bool OOBTree::remove_block_header(int bucket_index, int block_offset) {
    auto it = buckets.find(bucket_index);
    if (it == buckets.end()) return false;
    it->second.block_headers.at(block_offset).program_addr = -1;
    it->second.block_headers.at(block_offset).leaf = -1;
    return true;
}

bool OOBTree::is_dummy(int bucket_index, int block_offset) const {
    auto it = buckets.find(bucket_index);
    if (it == buckets.end()) throw "Bucket not found";
    return it->second.is_dummy(block_offset);
}

BlockHeader OOBTree::extract(int bucket_index, int block_offset) {
    return buckets.at(bucket_index).extract_header(block_offset);
}

void OOBTree::dump() const {
    std::cout << "ORAMTree dump: "<< buckets.size() <<" %zu buckets" << std::endl;
    for (const auto& [bidx, bucket] : buckets) {
        std::cout << "Bucket["<< bidx <<"] {" << std::endl;
        bucket.dump();
        std::cout << "}" << std::endl;
    }
}

}