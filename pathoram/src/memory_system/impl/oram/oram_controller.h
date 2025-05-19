#ifndef ORAM_BACKEND_H
#define ORAM_BACKEND_H

#include <list>
#include <unordered_map>

#include "base/base.h"
#include "base/request.h"
#include "base/clocked.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"
#include "memory_system/impl/oram/bucket_header.h"
#include "memory_system/impl/oram/oram_tree.h"


namespace Ramulator{

class ORAMController : public Clocked<ORAMController>{

        struct PosmapEntry{
            int block_id;
            uint64_t leaf;
        };
    
        struct StashEntry{
            int block_id;
            bool integrity_checked = false;
            bool writebacked = false;
            Addr_t writeback_addr;
        };

        /**
         * @brief Represents the current phase of an ORAM transaction.
         */
        enum class Phase { Reading, WaitingReadsDone, Writing };

        /**
         * @brief Represents a single ORAM transaction, including its state and metadata.
         * @details 
         * This structure holds all the necessary information to track the progress 
         * of an ORAM access, including its current phase, associated request, and 
         * outstanding read operations.
         * Used in tick() function.
         */
        struct ORAMTransaction{
            Phase phase;
            int n_read_ack;
            Request req;
            std::list<Addr_t> pending_read;
        };

    private:
        int required_acks;

        int pathoram_read_requests = 0;
        int pathoram_write_requests = 0;
        int pathoram_other_requests = 0;

        IAddrMapper* m_addr_mapper;
        std::vector<IDRAMController*> m_controllers;

        std::list<ORAMTransaction> oram_transactions;
        ORAMTransaction *current_transaction;

        //Position map: The key is the Address given by CPU
        std::unordered_map<Addr_t, PosmapEntry> position_map;
        //Stash: The Key is the memory Address (diffrerent from Position Map's Address)
        std::unordered_map<Addr_t, StashEntry> stash;
        
        //Out of band tree information
        ORAMTree *oram_tree;

        /**
         * @brief  Send the request to the real DRAM Controller
         */
        bool send_to_controller(Request &req);

        /**
         * @brief  Callback to be called when the controller completes a READ request
         * @details 
         * When a READ request is completed, the controller calls this callback to
         * update some ORAM Controller information and populate the stash.
         */
        void oram_read_callback(Request &r);

        /**
         * @brief  Returns a Position Map entry corresponding to the given block ID.
         * @details 
         * This method searches for and returns a pointer to the Position Map entry 
         * that matches the specified block ID. If no such entry exists, it may return nullptr.
         */
        PosmapEntry *find_posmap_entry_by_block_id(int block_id);
        StashEntry *find_stash_entry_by_block_id(int block_id);
        /**
         * @brief  Remaps the specified block ID to a new leaf in the ORAM tree.
         * @details 
         * This method updates the position map by assigning a new random leaf 
         * to the given block ID, effectively changing its location in the ORAM tree.
         * It is typically used after accessing a block to preserve obliviousness.
         */
        void remap(int block_id);

    public:    
        ORAMController(ORAMTree *oram_tree, IAddrMapper* m_addr_mapper, std::vector<IDRAMController*> m_controllers);

        /**
         * @brief  Advances the ORAM controller simulation by one clock cycle.
         * @details 
         * This method updates the internal state of the ORAM controller 
         * and processes any scheduled operations for the current cycle.
         */
        void tick();

        /**
         * @brief  Sends a request to the ORAM controller.
         * @details 
         * This method handles an incoming request (e.g., read or write) from the CPU
         * and returns whether the request was successfully buffered or processed.
         * @return true if the request was accepted, false otherwise.
         */
        bool send(Request req);

        /**
         * @brief  Allocates a new entry in the position map for a given address.
         * @details 
         * This method creates or updates a position map entry by associating 
         * an address with a block ID and a leaf in the ORAM tree.
         * 
         * @param addr The logical address of the data block.
         * @param block_id The unique identifier for the block.
         * @param leaf The leaf ID to which the block will be initially mapped.
         * @return A pair containing an iterator to the entry and a bool indicating 
         *         whether the insertion was successful (true if inserted, false if updated).
         */
        std::pair<std::unordered_map<Addr_t, PosmapEntry>::iterator, bool> posmap_allocate_entry(Addr_t addr, int block_id, uint64_t leaf);

        uint64_t posmap_get_entry_leaf(Addr_t addr);
        
        bool posmap_exists_entry(Addr_t addr) {
            return position_map.find(addr) != position_map.end();
        };

        bool stash_exists_entry(Addr_t addr) {
            int block_id = position_map.at(addr).block_id;
            StashEntry *se = find_stash_entry_by_block_id(block_id);
            return (se != nullptr);
        };
        /**
         * @brief  Returns the number of read requests handled by the ORAM controller.
         * @details 
         * Useful for profiling and performance analysis.
         * 
         * @return The number of read requests.
         */
        int get_num_read_reqs();
        
        /**
         * @brief  Returns the number of write requests handled by the ORAM controller.
         * @details 
         * Useful for profiling and performance analysis.
         * 
         * @return The number of write requests.
         */
        int get_num_write_reqs();

        /**
         * @brief  Returns the number of non-read/write requests handled.
         * @details 
         * This includes any request types not classified as read or write.
         * 
         * @return The number of other requests.
         */
        int get_num_other_reqs();

        /**
         * @brief  Prints the current contents of the position map.
         * @details 
         * This method is intended for debugging and visualization purposes.
         */
        void print_position_map();

        /**
         * @brief  Prints the current contents of the stash.
         * @details 
         * This method helps in inspecting the internal stash of blocks 
         * temporarily held during ORAM operations.
         */
        void print_stash();
};


}

#endif   // ORAM_BACKEND_H