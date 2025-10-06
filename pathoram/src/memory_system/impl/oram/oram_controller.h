#ifndef ORAM_BACKEND_H
#define ORAM_BACKEND_H

#include <queue>
#include <map>
#include <string>

#include "base/base.h"
#include "base/request.h"
#include "base/clocked.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"

#include "memory_system/impl/oram/oob/oob_tree.h"
#include "memory_system/impl/oram/oob/bucket.h"

#include "memory_system/impl/oram/components/interfaces/iposition_map.h"
#include "memory_system/impl/oram/components/interfaces/iaddress_logic.h"
#include "memory_system/impl/oram/components/interfaces/istash.h"
#include "memory_system/impl/oram/components/interfaces/ioram_controller.h"
#include "memory_system/impl/oram/components/interfaces/iintegrity_controller.h"

#include "memory_system/impl/oram/components/inc/oram_tree_info.h"

#include <fstream>

namespace Ramulator {

/**
 * @class ORAMController
 * @brief The main ORAM controller responsible for managing all subcomponents and data flow.
 *
 * The ORAMController is the core manager of ORAM operations. It contains
 * the Stash, Position Map, Address Logic, a Finite State Machine for each request,
 * a table of pending memory transactions, and a reference to the Out-of-Band ORAM structure.
 * 
 * Whenever a request is received from the CPU, it is enqueued in the transaction table.
 * If no other transactions are currently being executed, the next one in line is selected.
 * 
 * It handles remapping operations and ensures consistency between data structures.
 *
 * @note This controller can be extended for further improvements.
 */

class ORAMController : public IORAMController, Clocked<ORAMController> {
        /**
         * @brief Represents the current phase of an ORAM transaction.
         */
        enum class Phase {Pending, ReadingHeaders, ReadingData, Reply, WaitingReadsDone, Writing, WritebackDummy, WaitingWritesDone};

        struct TransactionEntry {
            Phase phase;
            Request req;
            Addr_t block_id;
            int n_acks;
            int leaf;
            Clk_t decrypt_cycle;
            bool integrity_checked;
            Clk_t arrival_time;
        };

        struct WriteRequest {
            Request req;
            Clk_t encrypt_cycle;
        };
    public:
        std::ofstream outdata;
        
    private:
        int level;
        int required_acks;
        Clk_t encrypt_delay;
        Clk_t decrypt_delay;

        //Counters
        size_t read_requests = 0;
        size_t write_requests = 0;
        size_t other_requests = 0;
        size_t num_stall_tick = 0;
        size_t cumulative_latency = 0;

        // ORAM Components
        IIntegrityController* integrity_controller;
        IAddrMapper* m_addr_mapper;
        std::vector<IDRAMController*> m_controllers;
        const ORAMTreeInfo* oram_tree_info;
        IPositionMap* position_map;
        IStash* stash;
        IAddressLogic* address_logic;
        
        // Transaction's queue
        std::queue<TransactionEntry> transaction_table;
        TransactionEntry* curr_transaction;

        // Requests' queue to memory
        std::queue<Request> pending_rd_reqs;

        std::queue<WriteRequest> pending_wb_reqs;
        
        //Out of band tree
        OOBTree oob_tree;

        /**
         * @brief  Send the request to the real DRAM Controller
         */
        bool send_to_controller(Request& req);

        /**
         * @brief When a block is received (dummy or data) from memory, it is decrypted.
         * This is modelled as an delay added to the current Clock cycle.
         */
        void decrypt_block();

        /**
         * @brief  Callback to be called when the DRAM Controller completes a READ request
         * @details 
         * When a READ request is completed, the controller calls this callback to
         * update some ORAM Controller information and populate the stash.
         */
        void oram_read_callback(Request& r);
        void oram_read_header_callback(Request& r);

        /**
         * @brief Selects the next transaction from the transaction table if none is currently active.
         *
         * If there is no current transaction in progress (`curr_transaction == nullptr`) and the
         * transaction table is not empty, the front transaction is selected for processing.
         *
         * @return `true` if a transaction is selected or already active, `false` if the transaction table is empty.
         */
        bool select_next_transaction();

        /**
         * @brief Processes any pending read requests in the queue.
         *        If a request is ready and the controller accepts it, it is removed from the queue.
         */
        void process_pending_reads();

        /**
         * @brief Processes any pending writeback requests in the queue.
         *        If a request is ready and the controller accepts it, it is sent and removed from the queue.
         */
        void process_pending_writes();

        /**
         * @brief Handles the phase where headers are being read from the ORAM tree.
         *        Requests are generated based on the transaction’s target leaf.
         *        Finally, the requests are buffered into the read queue.
         */
        void handle_reading_headers();

        /**
         * @brief Handles the phase where actual data blocks are being read.
         *        Requests are generated and callbacks attached for processing read completions.
         *        Finally, the requests are buffered into the read queue.
         */
        void handle_reading_data();

        /**
         * @brief Handles the phase where it has to wait the blocks to be read.
         */
        void handle_waiting_reads();

        /**
         * @brief After the reading phase, return the request block to the LLC.
         *        It should be in the stash.
         */
        void handle_reply_block();
        
        /**
         * @brief Handles the writing phase by selecting blocks from the stash and issuing write requests
         *        if a valid location on the path is available.
         *        The requests are buffered into the write queue.
         */
        void handle_writing_phase();
        void handle_writing_dummy();

        /**
         * @brief Finalizes the current transaction after all writebacks are completed.
         *        Removes the transaction from the table.
         */
        void handle_waiting_writes_done();

    public:
        ORAMController();

        ORAMController(int stash_size, Clk_t encrypt_delay, Clk_t decrypt_delay, IAddrMapper* m_addr_mapper,
                              std::vector<IDRAMController*> m_controllers);
        
        /**
         * @brief  Advances the ORAM controller simulation by one clock cycle.
         * @details 
         * This method updates the internal state of the ORAM controller 
         * and processes any scheduled operations for the current cycle.
         */
        void tick() override;

        /**
         * @brief  Sends a request to the ORAM controller.
         * @details 
         * This method handles an incoming request (e.g., read or write) from the CPU
         * and returns whether the request was successfully buffered or processed.
         * @return true if the request was accepted, false otherwise.
         */
        bool send(Request req) override;

        void connect_integrity_controller(IIntegrityController* integrity_controller) override;

        void integrity_check(Addr_t addr) override;

        void attach_oram_info(const ORAMTreeInfo* oram_tree_info) override;

        /**
         * @brief  Attach the PathORAM's access counter to ORAMCounter.
         */
        void set_counters(std::map<std::string, size_t&>& counters) override;
};


}

#endif   // ORAM_BACKEND_H