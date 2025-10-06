#ifndef I_POSITION_MAP_H
#define I_POSITION_MAP_H

#include "base/base.h"

namespace Ramulator {

/**
 * @class IPositionMap
 * @brief Interface class for Position Map
 */
class IPositionMap {
    public:
        IPositionMap() {};
        virtual ~IPositionMap() {};

        /**
         * @brief Adds a new entry to the position map.
         */
        virtual bool add_entry(Addr_t block_id, int leaf) = 0;

        /**
         * @brief Removes an entry from the position map.
         */
        virtual bool remove_entry(Addr_t block_id) = 0;

        /**
         * @brief Updates the leaf associated with a block identifier.
         */
        virtual bool remap(Addr_t block_id, int new_leaf) = 0;

        /**
         * @brief Retrieves the current leaf associated with a block identifier.
         */
        virtual int get_leaf(Addr_t block_id) = 0;

        /**
         * @brief Checks whether the position map contains a given block identifier.
         */
        virtual bool is_present(Addr_t block_id) = 0;

        /**
         * @brief Returns the number of entries currently stored in the position map.
         */
        virtual int get_num_entries() const = 0;

        /**
         * @brief Returns the number of remappings performed since initialization.
         */
        virtual int get_num_remappings() const = 0;

        /**
         * @brief  Prints the current contents of the position map.
         */
        virtual void dump() = 0;

};

}

#endif // I_POSITION_MAP_H