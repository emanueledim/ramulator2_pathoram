#ifndef I_STASH_H
#define I_STASH_H

#include "base/base.h"

#include "memory_system/impl/oram/oob/bucket.h"

namespace Ramulator {

/**
 * @class IStash
 * @brief Interface for the Stash component
 */
class IStash {

    public:
        IStash() {};
        virtual ~IStash() {};

        /**
         * @brief Adds a new entry to the stash.
         * @param block_header The block header to store. This represents the entire bucket.
         */
        virtual bool add_entry(BlockHeader block_header) = 0;

        /**
         * @brief Removes an entry from the stash by block id.
         * @param block_id The block id of the block to remove.
         */
        virtual bool remove_entry(Addr_t block_id) = 0;

        /**
         * @brief Updates the leaf associated with a given block id.
         * @param block_id The block id to remap.
         * @param new_leaf The new leaf index.
         */
        virtual bool remap(Addr_t block_id, int new_leaf) = 0;
        
        /**
         * @brief Retrieves the leaf associated with a given block id.
         * @param block_id The block id to query.
         */
        virtual int get_leaf(Addr_t block_id) = 0;

        /**
         * @brief Search the entry with key addr.
         * @param block_id The key to query.
         */
        virtual bool is_present(Addr_t block_id) = 0;

        /**
         * @brief Checks if the stash is empty.
         */
        virtual bool is_empty() = 0;

        /**
         * @brief Get the next BlockHeader.
         */
        virtual BlockHeader next() = 0;

        /**
         * @brief Resets the internal iterator or counter.
         */
        virtual void reset() = 0;
        
        /**
         * @brief Calculates the stash occupancy as a percentage of its capacity.
         */
        virtual float occupancy() = 0;
        
        /**
         * @brief Prints the current contents of the stash.
         */
        virtual void dump() = 0;
};

}

#endif   // I_STASH_H