#ifndef ORAM_TREE_INFO_H
#define ORAM_TREE_INFO_H

#include <random>

#include "base/base.h"

namespace Ramulator {

class ORAMTreeInfo {

    public:
        mutable std::mt19937 rng;
        mutable std::uniform_int_distribution<int> leaf_dist;

        //Tree's properties
        int tree_depth;
        int levels;
        int arity;
        
        //Tree-Memory properties
        Addr_t base_address_tree;
        Addr_t length_tree;
        int bucket_size;
        int block_size;
        int z_blocks;

        ORAMTreeInfo(Addr_t base_address_tree, Addr_t length_tree, int block_size, int z_blocks, int arity) :
            base_address_tree(base_address_tree), length_tree(length_tree), block_size(block_size),
            z_blocks(z_blocks), arity(arity) {
            bucket_size = block_size * z_blocks; 
            int num_buckets = (z_blocks/(z_blocks + 1.0) * length_tree) / bucket_size;
            int shift_bits_arity = static_cast<int>(std::log2(arity));
            int num_buckets2 = num_buckets + 1;
            tree_depth = 0;
            while(num_buckets2 >> shift_bits_arity) {
                tree_depth++;
                num_buckets2 >>= shift_bits_arity;
            }
            levels = tree_depth;
            tree_depth -= 1;
            std::random_device rd;
            rng.seed(rd());
            leaf_dist = std::uniform_int_distribution<int>(0, (int)pow(arity, tree_depth)-1);
        }

        /**
         * @brief Return a random leaf between 0 and max number of leaves.
         * @details This method returns a random leaf number generated through an uniform distribution between
         * 0 and the max number of precalculated leaves.
         */
        int get_random_leaf() const {
            return leaf_dist(rng);
        }

        /**
         * @brief Maps a memory address to the bucket index
         * @param addr the memory address to map
         * @return Returns the mapped `int bucket_index`
        */
        int get_bucket_index(Addr_t addr) const {
            return (addr - base_address_tree) / bucket_size;
        }

        /**
         * @brief Maps a memory address to the block offset within a bucket
         * @param addr the memory address to map
         * @return Returns the mapped `int block_offset` 0-based. Its range values space from 0 to `z_blocks-1`
        */
        int get_block_offset(Addr_t addr) const {
            int block_offset = (addr - base_address_tree) % (bucket_size);
            return block_offset / block_size;
        }
};

}

#endif // ORAM_TREE_INFO_H