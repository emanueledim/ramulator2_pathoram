#include "memory_system/impl/oram/oram_controller.h"

namespace Ramulator {

ORAMController::ORAMController(Addr_t max_paddr, int block_size, int z_blocks, int arity, int stash_size, Clk_t crypt_decrypt_delay,
                                IAddrMapper* m_addr_mapper, std::vector<IDRAMController*> m_controllers, IntegrityChecker* integrity_checker) {
  this->crypt_decrypt_delay = crypt_decrypt_delay;
  access_logic = AccessLogic(max_paddr, block_size, z_blocks, arity, &oob_tree);
  stash = Stash(stash_size);
  this->m_addr_mapper = m_addr_mapper;
  this->m_controllers = m_controllers;
  this->integrity_checker = integrity_checker;
  required_acks = z_blocks * (access_logic.get_tree_depth()+1);
}

bool ORAMController::send_to_controller(Request& req) {
  m_addr_mapper->apply(req);
  int channel_id = req.addr_vec[0];
  return m_controllers[channel_id]->send(req);
}

void ORAMController::decrypt_block() {
if(curr_transaction->decrypt_time > m_clk) {
    curr_transaction->decrypt_time += crypt_decrypt_delay;  
  } else {
    curr_transaction->decrypt_time = m_clk + crypt_decrypt_delay;
  }
}

void ORAMController::oram_read_callback(Request& r) {
  decrypt_block();
  curr_transaction->n_acks--;  
  // Get the bucket-block memory mapping
  int bucket_index = access_logic.get_bucket_index(r.addr);
  int block_offset = access_logic.get_block_offset(r.addr);

  // Get and remove the block from OOB Tree (Emulated DRAM Memory tree)
  BlockHeader block_header = oob_tree.extract(bucket_index, block_offset);

  // Check whether the block just read is dummy
  if(block_header.is_dummy()) return;
  
  // If it is not dummy, then store it in the stash
  if(!stash.add_entry(block_header)) return;

  // Enqueue in integrity checker
  integrity_checker->enqueue_block();
}

void ORAMController::oram_read_header_callback(Request& r) {
  decrypt_block();
}

bool ORAMController::select_next_transaction() {
  bool success = true;

  if(curr_transaction == nullptr) {
    if(transaction_table.empty()) {
      success = false;
    } else {
      curr_transaction = &transaction_table.front();
      // Get the effective leaf from the position map
      curr_transaction->leaf = position_map.get_leaf(curr_transaction->program_addr);
    }
  }
  return success;
}

void ORAMController::process_pending_reads() {
  if (!pending_rd_reqs.empty()) {
    Request& next_req = pending_rd_reqs.front();
    //TODO: possible improvement with multiqueue
    if (send_to_controller(next_req)) {
      pending_rd_reqs.pop();
      read_queue_stall = false;
      (*pathoram_read_requests)++;
    } else {
      read_queue_stall = true;
    }
  }
}

void ORAMController::process_pending_writes() {
  if (!pending_wb_reqs.empty()) {
    WriteRequest& next_req = pending_wb_reqs.front();
    if(m_clk > next_req.crypt_cycle) {
      //Block encrypted
      if (send_to_controller(next_req.req)) {
        pending_wb_reqs.pop();
        write_queue_stall = false;
        (*pathoram_write_requests)++;
      } else {
        write_queue_stall = true;
      }
    }
  }
}

void ORAMController::handle_reading_headers() {
  Addr_t next_addr = access_logic.generate_next_hdr_address(curr_transaction->leaf);
  if (next_addr != -1) {
    Request load_request(next_addr, Request::Type::Read);
    load_request.callback = [this](Request& req) {
      this->oram_read_header_callback(req);
    };
    pending_rd_reqs.push(load_request);
  } else {
    curr_transaction->phase = Phase::ReadingData;
  }
}

void ORAMController::handle_reading_data() {
  Addr_t next_addr = access_logic.generate_next_data_address(curr_transaction->leaf);
  if (next_addr != -1) {
    Request load_request(next_addr, Request::Type::Read);
    load_request.callback = [this](Request& req) {
      this->oram_read_callback(req);
    };
    pending_rd_reqs.push(load_request);
  } else {
    curr_transaction->phase = Phase::WaitingReadsDone;
  }
}

void ORAMController::handle_waiting_reads() {
  if (curr_transaction->n_acks <= 0) {
    //At this time, all the blocks have been received
    if(m_clk > curr_transaction->decrypt_time) {
      //Decrypt of all blocks terminated
      curr_transaction->phase = Phase::Reply;
    }
  }
}

void ORAMController::handle_reply_block() {
  if(stash.is_present(curr_transaction->program_addr)) {
    // To handle consequent requests for the same address, the
    // remapping procedure has to be placed here, after the reading
    // of all the blocks. Earlier or Later remappings will results
    // in inconsistency of leaf values stored in the different data structures.
    int new_leaf = access_logic.get_random_leaf();
    position_map.remap(curr_transaction->program_addr, new_leaf);
    stash.remap(curr_transaction->program_addr, new_leaf);
    access_logic.init_path(new_leaf);
    curr_transaction->req.callback(curr_transaction->req);
  } else {
    throw "Block not found in either stash or memory";
  }
  level = access_logic.get_tree_depth();
  curr_transaction->phase = Phase::Writing;
}

void ORAMController::handle_writing_phase() {
  //If the writeback queue is full, exit from the cycle
  if(write_queue_stall) return;

  if(stash.is_empty()) {
    level = access_logic.get_tree_depth();
    curr_transaction->phase = Phase::WaitingWritesDone;
    return;
  }

  auto stash_entry = stash.next();
  if(stash_entry == stash.end()) {
    level--;
    if(level < 0) {
      curr_transaction->phase = Phase::WaitingWritesDone;
    }
    return;
  }

  if(access_logic.is_common_bucket(curr_transaction->leaf, stash_entry->second, level)) {
    Addr_t wb_addr = access_logic.writeback_level(stash_entry->second, level, stash_entry->first);
    if(wb_addr != -1) {
      Request write_request(wb_addr, Request::Type::Write);
      Clk_t crypt_cycle = m_clk + crypt_decrypt_delay;
      pending_wb_reqs.push(WriteRequest(write_request, crypt_cycle));
      stash.erase();
    }
  }
}

void ORAMController::handle_waiting_writes_done() {
  if (pending_wb_reqs.empty()) {
    if (!transaction_table.empty() && curr_transaction != nullptr) {
      transaction_table.pop();
    }
    curr_transaction = nullptr;
  }
}

void ORAMController::tick() {
  m_clk++;

  process_pending_reads();
  process_pending_writes();
  
  if(!select_next_transaction()) {
    return;
  }  

  switch (curr_transaction->phase) {
    case Phase::Pending:
      curr_transaction->phase = Phase::ReadingHeaders;
      break;

    case Phase::ReadingHeaders:
      handle_reading_headers();
      break;

    case Phase::ReadingData:
      handle_reading_data();
      break;

    case Phase::WaitingReadsDone:
      handle_waiting_reads();
      break;

    case Phase::Reply:
      handle_reply_block();
      break;

    case Phase::Writing:
      handle_writing_phase();
      break;

    case Phase::WaitingWritesDone:
      handle_waiting_writes_done();
      break;

    default:
      break;
  }
}

bool ORAMController::send(Request req) {
  if(read_queue_stall) {
    return false;
  }

  //Out of band init 
  if(!position_map.is_present(req.addr)) {
    int leaf = access_logic.get_random_leaf();
    position_map.add_entry(req.addr, leaf);
    access_logic.init_path(leaf);
    if(!access_logic.insert_block_random_pos(req.addr, leaf)) exit(1);
  }

  TransactionEntry new_transaction_entry(Phase::Pending, req.addr, required_acks, -1, req, 0, 0);
  transaction_table.push(new_transaction_entry);
  return true;
}

void ORAMController::set_access_counters(int &pathoram_num_read_requests, int &pathoram_num_write_requests, int &pathoram_num_other_requests) {
  pathoram_read_requests = &pathoram_num_read_requests;
  pathoram_write_requests = &pathoram_num_write_requests;
  pathoram_other_requests = &pathoram_num_other_requests;
}

}   // namespace Ramulator