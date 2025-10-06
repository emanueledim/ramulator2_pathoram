#ifndef STASH_H
#define STASH_H

#include <unordered_map>

#include "base/base.h"

#include "memory_system/impl/oram/oob/bucket.h"
#include "memory_system/impl/oram/components/interfaces/istash.h"

namespace Ramulator {

/**
 * @class Stash
 * @brief Temporary storage structure used by the ORAM controller to hold data blocks during accesses.
 *
 * The element type stored is a BlockHeader entry, which represent the entire bucket.
 *  
 * The `Stash` class models a bounded temporary buffer that stores data blocks
 * which cannot yet be written back to the ORAM tree due to path constraints.
 * It supports insertion, removal, remapping of leaves, indexed access,
 * and provides occupancy statistics useful for analyzing ORAM behavior.
 *
 * The stash is internally implemented as an `unordered_map` where:
 * Key (`Addr_t`) is the block id.
 * Value (`int`) is the associated leaf node in the ORAM tree.
 *
 * Typical usage includes adding blocks after reads, holding them until a suitable
 * writeback slot becomes available on the path, and then removing them.
 *
 * @note The stash enforces a maximum size, beyond which no further entries can be added.
 */
class Stash : public IStash{

    private:
        int max_stash_size; // Maximum allowed number of entries in the stash.
        std::unordered_map<Addr_t, int> stash;
        std::unordered_map<Addr_t, int>::iterator current;

    public:
        Stash();
        Stash(int max_stash_size);

        /**
         * @brief Adds a new entry to the stash.
         * @param block_header The block header to store.
         * @return `true` if the entry was added; throws if the stash is full.
         */
        bool add_entry(BlockHeader block_header) override;

        /**
         * @brief Removes an entry from the stash by block id.
         * @param block_id The block id of the block to remove.
         * @return `true` if the entry was removed, `false` if not found.
         */
        bool remove_entry(Addr_t block_id) override;

        /**
         * @brief Updates the leaf associated with a given block id.
         * @param block_id The block id to remap.
         * @param new_leaf The new leaf index.
         * @return Always returns `true`.
         * @throws std::out_of_range if the block id is not present.
         */
        bool remap(Addr_t block_id, int new_leaf) override;
        
        /**
         * @brief Retrieves the leaf associated with a given block id.
         * @param block_id The block id to query.
         * @return The leaf index mapped to the block id.
         * @throws std::out_of_range if the block id is not present.
         */
        int get_leaf(Addr_t block_id) override;

        /**
         * @brief Search the entry with key block id.
         * @param block_id The key to query.
         * @return Returns `true` if the entry is found, `false` otherwise.
         */
        bool is_present(Addr_t block_id) override;

        /**
         * @brief Checks if the stash is empty.
         * @return `true` if stash contains no entries, `false` otherwise.
         */
        bool is_empty() override;
        
        /**
         * @brief Returns the next element the stash and advances the internal iterator.
         * This function allows `linear sequential iteration` over the elements stored in the stash.
         * Each call returns the element at the current position and advances the internal index.
         * Once the end of the stash is reached, it returns a BlockHeader with -1 values.
         * @return An iterator to the current element in the stash, or `stash.end()` if the stash is empty or fully traversed.
         */
        BlockHeader next() override;

        /**
         * @brief Resets the internal iterator to the begin of the stash.
         */
        void reset() override;

        /**
         * @brief Calculates the stash occupancy as a percentage of its capacity.
         * @return Occupancy percentage (0-100).
         */
        float occupancy() override;
        
        /**
         * @brief  Prints the current contents of the stash.
         * @details 
         * This method helps in inspecting the internal stash of blocks 
         * temporarily held during ORAM operations.
         */
        void dump() override;
};

}

#endif   // STASH_H