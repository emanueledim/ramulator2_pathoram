#ifndef ORAM_TREE_H
#define ORAM_TREE_H

#include <random>
#include <list>
#include <iostream>

#include "base/base.h"
#include "memory_system/impl/oram/bucket_header.h"

namespace Ramulator{

class ORAMTree {

    private:
        std::mt19937 rng;
        std::uniform_int_distribution<int> leaf_distribution;
        std::uniform_int_distribution<int> path_distribution;
        Addr_t max_paddr;
        uint64_t num_buckets;
        uint64_t num_leaves;
        uint32_t bucket_size;
        uint32_t block_size;
        uint32_t z_blocks;
        uint32_t tree_depth;
        
        int arity;
        std::unordered_map<int, BucketHeader> tree_metadata;
        
        int bucket_id_counter = 0;
        int block_id_counter = 0;

    public:

        ORAMTree(Addr_t max_paddr, uint32_t block_size, uint32_t z_blocks, int arity) :
            max_paddr(max_paddr), block_size(block_size), z_blocks(z_blocks), arity(arity) {
            bucket_size = block_size * z_blocks;

            num_buckets = max_paddr / bucket_size;
            num_leaves = (num_buckets + 1) / arity;
            tree_depth = static_cast<uint64_t>(std::log2(num_leaves));
            
            std::random_device rd;
            rng.seed(rd());
            leaf_distribution = std::uniform_int_distribution<int>(0, num_leaves);
            path_distribution = std::uniform_int_distribution<int>(0, (tree_depth * z_blocks) - 1);
        }

        /**
         * @brief  Retrieves the bucket header at the specified address.
         * @details 
         * Converts the address to a bucket index and returns a pointer to the corresponding
         * BucketHeader stored in the internal metadata.
         * 
         * @param addr The memory address of the bucket.
         * @return A pointer to the corresponding BucketHeader.
         */
        BucketHeader *get_bucket_header(Addr_t addr) {
            int bucket_index = get_bucket_index(addr);
            return &tree_metadata.at(bucket_index);
        }

        /**
         * @brief  Adds a new bucket header at the specified address.
         * @details 
         * Computes the bucket index from the address and inserts a new BucketHeader
         * into the tree metadata if it does not already exist.
         * 
         * @param addr The memory address where the new bucket header should be added.
         * @return true if the bucket header was successfully added; false if it already existed.
         */
        bool add_bucket_header(Addr_t addr) {
            int bucket_index = get_bucket_index(addr);
            if(tree_metadata.contains(bucket_index)) return false;
            BucketHeader bucket_header(bucket_id_counter++);
            tree_metadata.insert({bucket_index, bucket_header});
            return true;
        }
        
        /**
         * @brief  Computes the bucket index for a given address.
         * @details 
         * The index is calculated by dividing the address by the bucket size.
         * 
         * @param addr The memory address.
         * @return The index of the bucket that contains the address.
         */
        int get_bucket_index(Addr_t addr) {
            return addr / bucket_size;
        }

        /**
         * @brief  Computes the block offset within a bucket for a given address.
         * @details 
         * The offset is calculated by taking the modulo of the address with the bucket size,
         * and then dividing by the block size. This determines the position of the block
         * within the bucket.
         * 
         * @param addr The memory address.
         * @return The offset of the block within the corresponding bucket.
         */
        int get_block_offset(Addr_t addr) {
            return (addr % bucket_size) / block_size;
        }

        /**
         * @brief  Return a random leaf between 0 and max number of leaves
         * @details 
         * This method returns a random leaf number generated through an uniform distribution between
         * 0 and the max number of leaves precalculated in constructor in oram tree creation phase.
         */
        int get_random_leaf() {
            return leaf_distribution(rng);
        }

        /**
         * @brief  Returns the current block ID counter.
         * @details 
         * This method returns the current value of the block ID counter, which represents 
         * the ID that will be assigned to the next valid data block created.
         */
        int get_block_id() {
            return block_id_counter;
        }

        /**
         * @brief  Returns the depth of the ORAM tree.
         * @details 
         * The depth defines the number of levels in the tree structure, 
         * which affects the maximum number of leaves and the overall storage layout.
         * 
         * @return The tree depth as a 32-bit unsigned integer.
         */
        uint32_t get_tree_depth() {
            return tree_depth;
        }

        /**
         * @brief  Returns the number of data blocks per bucket (Z).
         * @details 
         * This value (Z) determines the capacity of each bucket in the ORAM tree, 
         * i.e., how many blocks can be stored in a single node.
         */
        uint32_t get_z_blocks() {
            return z_blocks;
        }

        /**
         * @brief  Return the addresses (the first block in) of every bucket
         * @details 
         * This method computes and returns the memory addresses of all blocks along
         * the path from the root to a given leaf in a binary tree used by a Path ORAM structure.
         * For each level of the tree (from the root to the specified leaf node), it calculates
         * the base address of the current bucket and adds the addresses of all Z blocks within
         * that bucket to a list. The traversal proceeds based on the parity of the current node index.
         */
        std::list<Addr_t> get_path_from_root(uint64_t leaf) {
            std::list<Addr_t> path_addresses;
            // Start from root (index 0)
            uint64_t current_index = 0;

            // Build the path from root to leaf
            for(int i=0; i < tree_depth; i++) {
                Addr_t base_address_bucket = current_index * bucket_size;
                for(int j=0; j < z_blocks; j++) {
                    path_addresses.push_back((Addr_t)(base_address_bucket + (block_size * j)));
                }

                /*
                uint64_t child_index = (leaf / arity) % arity + 1;
                current_index = current_index * arity + child_index;
                */
                if(leaf % 2 == 0) {
                    current_index = 2 * current_index + 1;
                } else {
                    current_index = 2 * current_index + 2;
                }
                leaf /= 2;
            }
            // Debug            
            /*for (auto addr : path_addresses)
                printf("%lu ", addr);
            printf("\nPath length: %lu\n", path_addresses.size() / z_blocks);*/
            return path_addresses;
        }

        /**
         * @brief  Set empty (in the Bucket Header) a block with a given (Memory) address.
         */
        void set_empty(Addr_t addr) {
            BucketHeader *bh = get_bucket_header(addr);
            int offset = get_block_offset(addr);
            bh->set_empty(offset);
        }

        /**
         * @brief  Init a path with (allocating) Bucket Headers given a leaf
         * @details 
         * Used by the PathORAM Object to initialize a path before ORAM Controller
         * uses it.
         * @param leaf The leaf number that defines the ORAM path to search.
         */
        void init_path(uint64_t leaf) {
            std::list<Addr_t> path = get_path_from_root(leaf);
            for (auto it : path) {
                add_bucket_header(it);
            }
        }

        /**
         * @brief  Initializes a new data block along the ORAM path to the specified leaf.
         * @details 
         * This method traverses the path from the root to the given leaf and attempts to 
         * insert a new data block into a randomly selected bucket along that path.
         * It continues searching until it finds an empty slot in a bucket. Once inserted,
         * the block is assigned the current value of the block ID counter, which is then incremented.
         *
         * @param leaf The leaf number that defines the ORAM path to search.
         * @return The address of the bucket where the new data block was successfully inserted.
         */
        Addr_t init_data_block(uint64_t leaf) {
            std::list<Addr_t> path = get_path_from_root(leaf);
            bool is_success = false;
            Addr_t data_block_addr = -1;
            while(!is_success) {
                auto it = path.begin();
                int block_index = path_distribution(rng);
                std::advance(it, block_index);
                Addr_t block_addr = *it;
                BucketHeader *bh = get_bucket_header(block_addr);
                int offset = get_block_offset(block_addr);
                if(bh->is_empty(offset)) {
                    bh->insert_data_block(offset, block_id_counter);
                    block_id_counter++;
                    data_block_addr = block_addr;
                    is_success = true;
                }
            }
            return data_block_addr;
        }

        /**
         * @brief  Inserts dummy blocks along the ORAM path to the given leaf.
         * @details 
         * This method randomly selects buckets along the path from the root to the specified leaf 
         * and attempts to insert dummy blocks into empty slots. It continues until the specified 
         * number of dummy blocks (or less) have been attempted (one per iteration, depending on availability).
         * @param leaf The leaf number that defines the ORAM path to search.
         * @param num_blocks The number of dummy block insertion attempts.
         */
        void init_dummy_blocks(uint64_t leaf, int num_blocks) {
            std::list<Addr_t> path = get_path_from_root(leaf);
            for(int i=0; i<num_blocks; i++) {
                auto it = path.begin();
                int dummy_index = path_distribution(rng);
                std::advance(it, dummy_index);
                Addr_t dummy_addr = *it;
                BucketHeader *bh = get_bucket_header(dummy_addr);
                int offset = get_block_offset(dummy_addr);
                if(bh->is_empty(offset)) {
                    bh->insert_dummy_block(offset);
                }
            }
        }

        /**
         * @brief  Attempts to insert a data block into the first available slot along an ORAM path.
         * @details 
         * This method traverses the path from the root to the specified leaf and inserts the 
         * block into the first empty slot it finds. If no available slot is found, the operation fails.
         * 
         * @param leaf The leaf number that defines the ORAM path to search.
         * @param block_id The identifier of the data block to be inserted.
         * @return true if the block was successfully inserted; false otherwise.
         */
        Addr_t insert_to_available_slot(uint64_t leaf, int block_id) {
            std::list<Addr_t> path = get_path_from_root(leaf);
            for (auto addr : path) {
                BucketHeader *bh = get_bucket_header(addr);
                int offset = get_block_offset(addr);
                if(bh->is_empty(offset)) {
                    bh->insert_data_block(offset, block_id);
                    return addr;
                }
            }
            return -1;
        }

};

}

#endif   // ORAM_TREE_H