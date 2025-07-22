#ifndef POSITION_MAP_H
#define POSITION_MAP_H

#include <unordered_map>

#include "base/base.h"

namespace Ramulator {

/**
 * @class PositionMap
 * @brief Maintains the logical-to-physical mapping of data blocks in the ORAM tree.
 *
 * The `PositionMap` stores and manages the current leaf assignment for each program-level
 * address (`Addr_t`) used in the ORAM system. Each time a block is accessed, it is remapped
 * to a new random leaf, and this mapping is recorded in the position map.
 *
 * Internally, the position map is implemented as an `unordered_map`:
 * Key (`Addr_t`): program address of the block.
 * Value (`int`): current leaf node in the ORAM tree.
 *
 * The class provides methods for:
 * Adding and removing entries.
 * Checking if an entry is present.
 * Remapping an address to a new leaf.
 * Retrieving the current leaf for a given address.
 *
 * @note This is a baseline linear mapping model
 */
class PositionMap {

    private:
        std::unordered_map<Addr_t, int> position_map;

        // Counters
        int num_entries = 0;
        int num_remappings = 0;

    public:
        PositionMap();

        /**
         * @brief Adds a new entry to the position map.
         * Associates a program address with a given leaf in the ORAM tree.
         * This should be called when a block is inserted for the first time.
         * @param program_addr The logical program address of the data block.
         * @param leaf The leaf index in the ORAM tree to which the block is assigned.
         */
        bool add_entry(Addr_t program_addr, int leaf);

        /**
         * @brief Removes an entry from the position map.
         * Deletes the mapping of a program address to its corresponding leaf.
         * @param program_addr The program address to be removed.
         * @return `true` if the entry existed and was removed, `false` otherwise.
         * @note This is usually not used for linear position map (no need for capacity management).
         */
        bool remove_entry(Addr_t program_addr);

        /**
         * @brief Updates the leaf associated with a program address.
         * This is typically used to assign a new random leaf after each access.
         * @param program_addr The address whose mapping is to be updated.
         * @param new_leaf The new leaf index to associate with the address.
         * @throws std::out_of_range if the address is not present in the map.
         */
        bool remap(Addr_t program_addr, int new_leaf);

        /**
         * @brief Retrieves the current leaf associated with a program address.
         * @param program_addr The program address to query.
         * @return The leaf index currently assigned to the given address.
         * @throws std::out_of_range if the address is not present in the map.
         */
        int get_leaf(Addr_t program_addr);

        /**
         * @brief Checks whether the position map contains a given address.
         * @param program_addr The address to check.
         * @return `true` if the address is present, `false` otherwise.
         */
        bool is_present(Addr_t program_addr);

        /**
         * @brief Returns the number of entries currently stored in the position map.
         * @return The number of (address, leaf) pairs currently in the map.
         */
        int get_num_entries() const;

        /**
         * @brief Returns the number of remappings performed since initialization.
         * @return The total number of remap operations executed on the position map.
         */
        int get_num_remappings() const;

        /**
         * @brief  Prints the current contents of the position map.
         */
        void dump();

};

}

#endif   // POSITION_MAP_H