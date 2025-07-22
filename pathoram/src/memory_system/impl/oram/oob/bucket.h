#ifndef BUCKET_H
#define BUCKET_H

#include "base/base.h"

#include <vector>

namespace Ramulator {

class OOBTree;

/**
 * @class Block
 * @brief Represents a generic DRAM block.
 * 
 * This is used as base class for Header and Data blocks.
 */
class Block {
    public:
        Block() = default;
};

/**
 * @class BlockHeader
 * @brief Represents metadata for a block in an ORAM bucket.
 *
 * This structure stores the program address and the corresponding leaf
 * in the ORAM tree where the block is logically mapped.
 */
class BlockHeader : public Block {
    public:
        Addr_t program_addr = -1;
        int leaf = -1;

        BlockHeader() = default;

        /**
         * @brief Parameterized constructor to create a block header.
         * @param program_addr The logical address of the block.
         * @param leaf The ORAM leaf associated with the block.
         */
        BlockHeader(Addr_t program_addr, int leaf) : program_addr(program_addr), leaf(leaf) {}

        bool is_dummy() { return program_addr < 0; }
};

/**
 * @class BlockData
 * @brief Represents the block's data.
 * 
 * This structure stores a data stub block.
 */
class BlockData : public Block {
    public:
        char data;

        BlockData() = default;
};

/**
 * @class Bucket
 * @brief Represents a physical bucket in the ORAM tree structure.
 *
 * A bucket contains a fixed number of block headers (`z_blocks`) and provides
 * methods for inserting, removing, and querying metadata about the blocks.
 * Each block is represented only by its header (address and leaf).
 */
class Bucket {

    private:
        std::vector<BlockHeader> block_headers;
        std::vector<BlockData> block_data;  //TODO: Unused. Not useful now.
    
        /**
         * @brief Retrieves a pointer to the block header at the specified offset.
         * @param offset The index of the block header within the bucket.
         * @return Pointer to the block header, or `nullptr` if offset is invalid.
         */
        BlockHeader get_block_header(int offset) {
            return block_headers.at(offset);
        }

        /**
         * @brief Returns a copy and remove the header of the block at the given `offset`.
         * @param offset The index of the block header within the bucket.
         * @return A copy of `BlockHeader`; or throws if the block_header does not exists.
         */
        BlockHeader extract_header(int offset) {
            BlockHeader block_header = block_headers.at(offset);
            block_headers.at(offset) = BlockHeader();
            return block_header;
        }

        /**
         * @brief Checks whether the block at the given offset in the bucket is a dummy.
         * @param offset Index of the block in the bucket.
         * @return `true` if the block is a dummy (invalid), `false` otherwise.
         */
        bool is_dummy(int offset) const {
            return block_headers[offset].program_addr < 0;
        }

        friend class OOBTree;

    public:

        /**
         * @brief Constructs a bucket with space for `z_blocks` default headers.
         * @param z_blocks The number of slots in the bucket.
         */
        Bucket(int z_blocks): block_headers(z_blocks) {};

        /**
         * @brief Gets the number of block headers in the bucket.
         * @return Total number of slots (capacity).
         */
        int size() const {
            return static_cast<int>(block_headers.size());
        }

        /**
         * @brief Inserts a block header into the specified offset.
         * @param block_offset Index in the bucket to insert into.
         * @param program_addr The address of the block.
         * @param leaf The associated leaf.
         * @return `true` if the insertion was successful; `false` otherwise.
         */
        bool insert_block_header(int block_offset, Addr_t program_addr, int leaf) {
            if (block_offset >= 0 && block_offset < static_cast<int>(block_headers.size())) {
                block_headers[block_offset].program_addr = program_addr;
                block_headers[block_offset].leaf = leaf;
                return true;
            }
            return false;
        }

        /**
         * @brief Removes a block header at the specified offset.
         * @param block_offset The index of the block to remove.
         * @return `true` if the block was removed; `false` if offset is invalid.
         */
        bool remove_block_header(int block_offset) {
            if (block_offset >= 0 && block_offset < static_cast<int>(block_headers.size())) {
                block_headers[block_offset] = BlockHeader();
                return true;
            }
            return false;
        }

        /**
         * @brief Prints the contents of the bucket.
         */
        void dump() const {
            for (size_t i = 0; i < block_headers.size(); ++i) {
                const auto& hdr = block_headers[i];
                std::cout<<" ["<< i<<"] addr: "<< hdr.program_addr <<", leaf: "<< hdr.leaf << std::endl;
            }
        }
};

}

#endif  //BUCKET_H