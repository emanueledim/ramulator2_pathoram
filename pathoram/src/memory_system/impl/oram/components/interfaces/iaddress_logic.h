#ifndef I_ADDRESS_LOGIC_H
#define I_ADDRESS_LOGIC_H

#include "base/base.h"

#include "memory_system/impl/oram/components/inc/oram_tree_info.h"

namespace Ramulator {

/**
 * @class IAddressLogic
 * @brief Interface for Address Logic class
 */
class IAddressLogic {
        
    public:
        IAddressLogic() {};
        virtual ~IAddressLogic() {};

        /**
         * @brief Returns the next available data block address along the path to the
         * given leaf.
         * @param leaf Target leaf index.
         */
        virtual Addr_t generate_next_address(int leaf) = 0;

        /**
         * @brief Returns the next available header address along the path to the
         * given leaf.
         * @param leaf Target leaf index.
         */
        virtual Addr_t generate_next_hdr_address(int leaf) = 0;

        /**
         * @brief Initializes all buckets in the `oob_oram` structure along the path from
         * the root to the specified leaf.
         * @param leaf Index of the leaf in the ORAM tree.
         */
        virtual void init_path(int leaf) = 0;

        /**
         * @brief Inserts a block into a random bucket along the path to the given leaf.
         * If found a slot is found, this will inserts the block header there.
         * @param block_id The block id of the block.
         * @param leaf Target leaf index in the ORAM tree.
         */
        virtual bool init_block(Addr_t block_id, int leaf) = 0;

        /**
         * @brief Determines whether the buckets at the given `level` are the same
         * in the paths to `leaf1` and `leaf2`.
         * @param leaf1 The first leaf node.
         * @param leaf2 The second leaf node.
         * @param level The level in the tree to check for a common bucket.
         */
        virtual bool is_common_bucket(int leaf1, int leaf2, int level) = 0;

        /**
         * @brief Tries to writebacks a block (block_id and leaf) in the corresponding
         * bucket `level` on `leaf`'s path.
         * If the bucket is not full, the corresponding bucket in OOBTree is updated.
         * @param leaf Leaf used to specify the path.
         * @param level Level of the path in which this tries to writeback the block.
         * @param block_id The block id of the block to writeback.
         */
        virtual Addr_t writeback_data(int leaf, int level, Addr_t block_id) = 0;

        /**
         * @brief Tries to writebacks a dummy in the corresponding
         * bucket `level` on `leaf`'s path. This will not overwrite the OOB structure.
         */
        virtual Addr_t writeback_dummy(int leaf, int level) = 0;

        /**
         * @brief Dependency Injection of the object that holds information about
         * the ORAM Tree. 
         */
        virtual void attach_oram_info(const ORAMTreeInfo* oram_tree_info) = 0;
};

}

#endif  // I_ADDRESS_LOGIC_H