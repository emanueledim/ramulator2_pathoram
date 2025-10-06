#ifndef ADDRESS_LOGIC_H
#define ADDRESS_LOGIC_H

#include <random>
#include <list>

#include "base/base.h"

#include "memory_system/impl/oram/oob/bucket.h"
#include "memory_system/impl/oram/oob/oob_tree.h"
#include "memory_system/impl/oram/components/interfaces/iaddress_logic.h"
#include "memory_system/impl/oram/components/inc/oram_tree_info.h"

namespace Ramulator {

/**
 * @class AddressLogicDoubleTree
 * @brief Generates the memory address to access based on the leaf received.
 * 
 * Typically, this class generates generate a list of addresses used to access the memory in
 * a linearized Tree fashion, as simple first assumption. Through the constructor,
 * it is possible to configure the memory space, size of bucket and other parameters.
 * The ORAMController must provide to this class the ORAM Out of band structure.
 *
 * The class provides methods for:
 * Getting information about the ORAM Tree structure.
 * Initialize a path for the Out of band ORAM feature.
 * Insert blocks into random buckets along a path in order to define the initial state.
 * Generate addresses for header and data tree structure.
 * Provide a function to writeback a block, based on the PathORAM's feature.
 */
class AddressLogicDoubleTree : public IAddressLogic {

    private:
        const ORAMTreeInfo* oram_tree_info;
        OOBTree* oob_tree;

        //Rng
        std::mt19937 rng;

        Addr_t base_address_headers_tree;

        //Util
        int cnt_addr = 0;
        int dummy_wb = 0;


    protected:
        /**
         * @brief Generates the addresses for the blocks along the path specified by the leaf in the ORAM data tree.
         * @param leaf Leaf index used to access the data path. 
         * @return Returns a `std::list<Addr_t>` containing the addresses of the blocks along the path from
         * 0-based root to the leaf.
         */
        std::list<Addr_t> access_data_path(int leaf);

        /**
         * @brief Generates the addresses for the blocks along the path specified by the leaf in the ORAM header tree.
         * @param leaf Leaf index used to access the header path. 
         * @return Returns a `std::list<Addr_t>` containing the addresses of the block headers along the path from
         * 0-based root to the leaf.
         */
        std::list<Addr_t> access_headers_path(int leaf);

        /**
         * @brief Computes all node indexes along the path from root to the given leaf.
         * Traverses the ORAM tree from the specified leaf up to the root and
         * collects the indexes of each bucket along the path.
         * @param leaf Leaf index in the ORAM tree.
         * @return List of bucket indexes from root to leaf.
         */
        std::list<int> get_path_indexes(int leaf);
        
    public:
        AddressLogicDoubleTree() = default;

        AddressLogicDoubleTree(OOBTree* oob_tree);        

        /**
         * @brief Returns the next available header address along the path to the
         * given leaf based on an internal counter. Uses internal counter `cnt_addr`
         * to iterate sequentially through header addresses.
         * Resets the counter and returns -1 if the end is reached.
         * @param leaf Target leaf index.
         * @return The next header address, or -1 if it reached the end.
         */
        Addr_t generate_next_hdr_address(int leaf) override;
        
        /**
         * @brief Returns the next available data block address along the path to the
         * given leaf based on an internal counter. Uses internal counter `cnt_addr`
         * to iterate sequentially through data block addresses.
         * Resets the counter and returns -1 if the end is reached.
         * @param leaf Target leaf index.
         * @return The next data block address, or -1 if it reached the end.
         */
        Addr_t generate_next_address(int leaf) override;

        /**
         * @brief Initializes all buckets in the `oob_oram` structure along the path from
         * the root to the specified leaf.
         * This function takes a leaf index and constructs the path from the root node
         * to that leaf. For each index along this path, it attempts to insert a new
         * bucket initialized with the `z_blocks` into the `oob_tree` structure.
         * If a bucket is already present at a given position, the insertion is skipped
         * to avoid overwriting existing data.
         * @param leaf Index of the leaf in the ORAM tree.
         * @note The path must be initialized before accessing any leaf.
         */
        void init_path(int leaf) override;

        /**
         * @brief Inserts a block into a random bucket along the path to the given leaf.
         * Tries up to 100 times to find a free slot (dummy) in a random bucket on the path
         * from the root to the specified leaf. If found, inserts the block header there.
         * @param block_id The block id of the block.
         * @param leaf Target leaf index in the ORAM tree.
         * @return `true` if insertion succeeds, `false` otherwise.
         */
        bool init_block(Addr_t block_id, int leaf) override;

        /**
         * @brief Determines whether the buckets at the given `level` are the same 
         *        in the paths to `leaf1` and `leaf2`.
         * This function checks if the paths from the root to `leaf1` and `leaf2` 
         * share a common bucket at the specified tree level.
         * @param leaf1 The first leaf node.
         * @param leaf2 The second leaf node.
         * @param level The level in the tree to check for a common bucket.
         * @return `true` if both paths include the same bucket at the given level; 
         *         `false` otherwise.
         */
        bool is_common_bucket(int leaf1, int leaf2, int level) override;

        /**
         * @brief Tries to writebacks a block (block_id and leaf) in the corresponding bucket `level` on `leaf`'s path.
         * It checks whether the bucket at the corresponding level is full.
         * If it is not full, the corresponding bucket in OOBTree is updated.
         * @param leaf Leaf used to specify the path.
         * @param level Level of the path in which this tries to writeback the block.
         * @param block_id The block id address of the block to writeback.
         * @return `-1` if the bucket is full; or the corresponding memory `Addr_t` (> -1) if the writeback is successful.
         */
        Addr_t writeback_data(int leaf, int level, Addr_t block_id) override;
        
        /**
         * @brief Performs a writeback of a dummy block (block_id and leaf) in the corresponding bucket `level` on `leaf`'s path.
         * The bucket should not be full.
         * @param leaf Leaf used to specify the path.
         * @param level Level of the path in which this tries to writeback the block.
         * @return `-1` if the bucket is full; or the corresponding memory `Addr_t` (> -1) if the writeback is successful.
         */
        Addr_t writeback_dummy(int leaf, int level) override;

        /**
         * @brief Dependency Injection of the object that holds information about
         * the ORAM Tree. 
         */
        void attach_oram_info(const ORAMTreeInfo* oram_tree_info) override;
};

}

#endif  // ADDRESS_LOGIC_H