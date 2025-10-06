#include "memory_system/impl/oram/oram_controller.h"
#include "memory_system/impl/oram/components/inc/address_logic_double_tree.h"
#include "memory_system/impl/oram/components/inc/stash.h"
#include "memory_system/impl/oram/components/inc/position_map.h"

namespace Ramulator {

ORAMController::ORAMController() { }

ORAMController::ORAMController(int stash_size, Clk_t encrypt_delay, Clk_t decrypt_delay, IAddrMapper* m_addr_mapper,
                              std::vector<IDRAMController*> m_controllers) {
  this->encrypt_delay = encrypt_delay;
  this->decrypt_delay = decrypt_delay;
  address_logic = new AddressLogicDoubleTree(&oob_tree);
  stash = new Stash(stash_size);
  position_map = new PositionMap();
  this->m_addr_mapper = m_addr_mapper;
  this->m_controllers = m_controllers;
}

bool ORAMController::send_to_controller(Request& req) {
  m_addr_mapper->apply(req);
  int channel_id = req.addr_vec[0];
  return m_controllers[channel_id]->send(req);
}

void ORAMController::decrypt_block() {
if(curr_transaction->decrypt_cycle > m_clk) {
    curr_transaction->decrypt_cycle += decrypt_delay;  
  } else {
    curr_transaction->decrypt_cycle = m_clk + decrypt_delay;
  }
}

void ORAMController::oram_read_callback(Request& req) {
  decrypt_block();
  curr_transaction->n_acks--;  
  integrity_controller->enqueue_block(req);

  // Get the bucket-block memory mapping
  int bucket_index = oram_tree_info->get_bucket_index(req.addr);
  int block_offset = oram_tree_info->get_block_offset(req.addr);

  // Get and remove the block from OOB Tree (Emulated DRAM Memory tree)
  BlockHeader block_header = oob_tree.pop(bucket_index, block_offset);

  // Check whether the block just read is dummy
  if(block_header.is_dummy()) return;
  
  // If it is not dummy, then store it in the stash
  if(!stash->add_entry(block_header)) return;
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
      curr_transaction->leaf = position_map->get_leaf(curr_transaction->block_id);
      outdata << m_clk << "," << stash->occupancy() << std::endl;
    }
  }
  return success;
}

void ORAMController::process_pending_reads() {
  //
  if (!pending_rd_reqs.empty()) {
    Request& next_req = pending_rd_reqs.front();
    if (send_to_controller(next_req)) {
      pending_rd_reqs.pop();
      read_requests++;
    } else {
      num_stall_tick++;
    }
  }
}

void ORAMController::process_pending_writes() {
  if (!pending_wb_reqs.empty()) {
    WriteRequest& next_req = pending_wb_reqs.front();
    if(m_clk > next_req.encrypt_cycle) {
      //Block encrypted
      if (send_to_controller(next_req.req)) {
        pending_wb_reqs.pop();
        write_requests++;
      } else {
        num_stall_tick++;
      }
    }
  }
}

void ORAMController::handle_reading_headers() {
  Addr_t next_addr = address_logic->generate_next_hdr_address(curr_transaction->leaf);
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
  Addr_t next_addr = address_logic->generate_next_address(curr_transaction->leaf);
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
    if(m_clk > curr_transaction->decrypt_cycle && curr_transaction->integrity_checked) {
      //Decrypt of all blocks terminated
      curr_transaction->phase = Phase::Reply;
    }
  }
}

void ORAMController::handle_reply_block() {
  if(stash->is_present(curr_transaction->block_id)) {
    // To handle consequent requests for the same address, the
    // remapping procedure has to be placed here, after the reading
    // of all the blocks. Earlier or Later remappings will results
    // in inconsistency of leaf values stored in the different data structures.
    int new_leaf = oram_tree_info->get_random_leaf();
    position_map->remap(curr_transaction->block_id, new_leaf);
    stash->remap(curr_transaction->block_id, new_leaf);
    address_logic->init_path(new_leaf);
    curr_transaction->req.callback(curr_transaction->req);
    level = oram_tree_info->tree_depth;
    stash->reset();
    curr_transaction->phase = Phase::Writing;
    cumulative_latency += m_clk - curr_transaction->arrival_time;
  } else {
    throw "Block not found in either stash or memory";
  }
}

void ORAMController::handle_writing_phase() {
  if(stash->is_empty()) {
    curr_transaction->phase = Phase::WritebackDummy;
    return;
  }

  BlockHeader stash_entry = stash->next();
  if(stash_entry.block_id == -1) {
    curr_transaction->phase = Phase::WritebackDummy;
  }
  
  int entry_block_id = stash_entry.block_id;
  int entry_leaf = stash_entry.leaf;
  if(address_logic->is_common_bucket(curr_transaction->leaf, entry_leaf, level)) {
    Addr_t wb_addr = address_logic->writeback_data(entry_leaf, level, entry_block_id);
    if(wb_addr != -1) {
      Request write_request(wb_addr, Request::Type::Write);
      Clk_t encrypt_cycle = m_clk + encrypt_delay;
      pending_wb_reqs.push(WriteRequest(write_request, encrypt_cycle));
      stash->remove_entry(entry_block_id);
    }
  }
}

void ORAMController::handle_writing_dummy() {
  Addr_t wb_addr = address_logic->writeback_dummy(curr_transaction->leaf, level);
  if(wb_addr >= 0) {
    Request write_request(wb_addr, Request::Type::Write);
    Clk_t encrypt_cycle = m_clk + encrypt_delay;
    pending_wb_reqs.push(WriteRequest(write_request, encrypt_cycle));
  } else {
    level--;
    if(level < 0) {
      //printf("Stash occupancy %f\n", stash->occupancy());
      curr_transaction->phase = Phase::WaitingWritesDone;
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

    case Phase::WritebackDummy:
      handle_writing_dummy();
      break;

    case Phase::WaitingWritesDone:
      handle_waiting_writes_done();
      break;

    default:
      break;
  }
}

bool ORAMController::send(Request req) {
  //Out of band init
  if(!position_map->is_present(req.addr)) {
    int leaf = oram_tree_info->get_random_leaf();
    position_map->add_entry(req.addr, leaf);
    address_logic->init_path(leaf);
    if(!address_logic->init_block(req.addr, leaf)) exit(1);
  }

  TransactionEntry new_transaction_entry(Phase::Pending, req, req.addr, required_acks, -1, 0, false, m_clk);
  transaction_table.push(new_transaction_entry);
  return true;
}

void ORAMController::connect_integrity_controller(IIntegrityController* integrity_controller) {
  this->integrity_controller = integrity_controller;
};

void ORAMController::integrity_check(Addr_t addr) {
  curr_transaction->integrity_checked = true;
};
        
void ORAMController::attach_oram_info(const ORAMTreeInfo* oram_tree_info) {
  this->oram_tree_info = oram_tree_info;
  address_logic->attach_oram_info(this->oram_tree_info);
  required_acks = oram_tree_info->z_blocks * oram_tree_info->levels;
}

void ORAMController::set_counters(std::map<std::string, size_t&>& counters) {
  counters.insert({"oram_controller_read_requests", read_requests});
  counters.insert({"oram_controller_write_requests", write_requests});
  counters.insert({"oram_controller_other_requests", other_requests});
  counters.insert({"oram_controller_num_stall_tick", num_stall_tick});
  counters.insert({"oram_controller_cumulative_latency", cumulative_latency});
  
}

}   // namespace Ramulator