#ifndef OOB_TREE_H
#define OOB_TREE_H

#include <map>

#include "base/base.h"
#include "memory_system/impl/oram/oob/bucket.h"

namespace Ramulator{

/**
 * @class OOBTree
 * @brief Represents the logical structure of an Out of Band ORAM tree composed of buckets.
 *
 * This class manages a set of buckets that form the ORAM tree. Each bucket
 * stores metadata about blocks in the form of `BlockHeader` objects.
 * The tree supports insertion, removal, and lookup operations on block metadata.
 * This is an out of band component.
 */
class OOBTree {

    private:
        std::map<int, Bucket> buckets;

    public:
        OOBTree() = default;
        
        /**
         * @brief Inserts a new bucket into the tree at the given index.
         * @param bucket_index The index of the node in the tree.
         * @param bucket The bucket to insert.
         * @return `true` if insertion was successful, `false` if the index already exists.
         */
        bool insert_bucket(int bucket_index, const Bucket& bucket);
            
        /**
         * @brief Inserts a block header into a specific bucket and offset.
         * @param bucket_index Index of the bucket in the tree.
         * @param block_offset Index of the slot within the bucket.
         * @param block_header The `BlockHeahder` to insert into the bucket.
         * @return `true` if the insertion was successful, `false` otherwise.
         */
        bool insert_block_header(int bucket_index, int block_offset, BlockHeader block_header);

        /**
         * @brief Removes a block header from a specific bucket and offset.
         * This sets the leaf and block_id to value -1 (dummy block).
         * Does not phisically remove the block.
         * @param bucket_index Index of the bucket.
         * @param block_offset Offset within the bucket.
         * @return `true` if the block existed and was removed; `false` otherwise.
         */
        bool remove_block_header(int bucket_index, int block_offset);

        /**
         * @brief Checks whether a block in the given location is a dummy.
         * @param bucket_index Index of the bucket.
         * @param block_offset Offset within the bucket.
         * @return `true` if the block is a dummy; `false` otherwise.
         */
        bool is_dummy(int bucket_index, int block_offset) const;

        /**
         * @brief Retrieves the BlockHeader at the specified bucket_index and block_offset.
         * @param bucket_index Index of the bucket.
         * @param block_offset Offset within the bucket.
         * @return The BlockHeader, or if the block is dummy.
         */
        BlockHeader pop(int bucket_index, int block_offset);
        
        /**
         * @brief Prints the full contents of the ORAM tree.
         */
        void dump() const;
};

}

#endif   // OOB_TREE_H