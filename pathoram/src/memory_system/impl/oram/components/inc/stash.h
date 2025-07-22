#ifndef STASH_H
#define STASH_H

#include <unordered_map>

#include "base/base.h"

#include "memory_system/impl/oram/oob/bucket.h"

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
 * Key (`Addr_t`) is the block’s address (the program address).
 * Value (`int`) is the associated leaf node in the ORAM tree.
 *
 * Typical usage includes adding blocks after reads, holding them until a suitable
 * writeback slot becomes available on the path, and then removing them.
 *
 * @note The stash enforces a maximum size, beyond which no further entries can be added.
 */
class Stash {

    private:
        int max_stash_size; // Maximum allowed number of entries in the stash.
        std::unordered_map<Addr_t, int> stash;
    
        size_t current_index = 0;
        size_t last_index = 0;
    public:
        Stash();
        Stash(int max_stash_size);

        /**
         * @brief Adds a new entry to the stash.
         * @param block_header The block header to store.
         * @return `true` if the entry was added; throws if the stash is full.
         */
        bool add_entry(BlockHeader block_header);

        /**
         * @brief Removes an entry from the stash by program address.
         * @param program_addr The address of the block to remove.
         * @return `true` if the entry was removed, `false` if not found.
         */
        bool remove_entry(Addr_t program_addr);

        /**
         * @brief Updates the leaf associated with a given program address.
         * @param program_addr The address to remap.
         * @param new_leaf The new leaf index.
         * @return Always returns `true`.
         * @throws std::out_of_range if the address is not present.
         */
        bool remap(Addr_t program_addr, int new_leaf);
        
        /**
         * @brief Search the entry with key addr.
         * @param program_addr The key to query.
         * @return Returns `true` if the entry is found, `false` otherwise.
         */
        bool is_present(Addr_t program_addr);

        /**
         * @brief Retrieves the leaf associated with a given program address.
         * @param program_addr The address to query.
         * @return The leaf index mapped to the address.
         * @throws std::out_of_range if the address is not present.
         */
        int get_leaf(Addr_t program_addr);

        /**
         * @brief Checks if the stash is empty.
         * @return `true` if stash contains no entries, `false` otherwise.
         */
        bool is_empty();

        /**
         * @brief Returns an iterator to the end of the stash.
         * @return Iterator pointing past the last element.
         */
        std::unordered_map<Addr_t, int>::iterator end();

        /**
         * @brief Returns the current element in the stash and advances the internal iterator.
         * This function allows `linear sequential iteration` over the elements stored in the stash.
         * Each call returns the element at the current position and advances the internal index.
         * Once the end of the stash is reached, it returns `stash.end()` and resets the iteration.
         *
         * @note If the stash is empty or the end is reached, `stash.end()` is returned.
         *       Internally, `last_index` is updated to track the most recently returned element.
         *
         * @return An iterator to the current element in the stash, or `stash.end()` if the stash is empty or fully traversed.
         */
        std::unordered_map<Addr_t, int>::iterator next();

        /**
         * @brief Return the current element pointed by the internal iterator.
         * @return The `stash.end()` iterator if the stash is empty, otherwise the `current` element.
         */
        std::unordered_map<Addr_t, int>::iterator current();

        /**
         * @brief Removes the current element from the stash.
         * It is pointed by an internal iterator. The element is 
         * the one returned by current() method.
         * @return `true` if removal succeeded, `false` otherwise (Stash empty).
         */
        bool erase();
        
        /**
         * @brief Calculates the stash occupancy as a percentage of its capacity.
         * @return Occupancy percentage (0-100).
         */
        float occupancy();
        
        /**
         * @brief  Prints the current contents of the stash.
         * @details 
         * This method helps in inspecting the internal stash of blocks 
         * temporarily held during ORAM operations.
         */
        void dump();
};

}

#endif   // STASH_H