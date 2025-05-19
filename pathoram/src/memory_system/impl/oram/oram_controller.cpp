#include <iostream>
#include <stdexcept>
#include "memory_system/impl/oram/oram_controller.h"


namespace Ramulator {

ORAMController::ORAMController(ORAMTree *oram_tree, IAddrMapper* m_addr_mapper, std::vector<IDRAMController*> m_controllers) {
    this->oram_tree = oram_tree;
    this->m_addr_mapper = m_addr_mapper;
    this->m_controllers = m_controllers;
    required_acks = oram_tree->get_z_blocks() * oram_tree->get_tree_depth();
}

bool ORAMController::send_to_controller(Request &req) {
  m_addr_mapper->apply(req);
  int channel_id = req.addr_vec[0];
  return m_controllers[channel_id]->send(req);
}

void ORAMController::oram_read_callback(Request& r) {
  // When a READ is completed, increment the ack counter and write it to the stash.
  // Set it to empty in the Bucket Header.
  current_transaction->n_read_ack++;
  BucketHeader *bh = oram_tree->get_bucket_header(r.addr);
  int offset = oram_tree->get_block_offset(r.addr);
  stash.emplace(r.addr, StashEntry{bh->get_block_ids()[offset]});
  oram_tree->set_empty(r.addr);
}

ORAMController::PosmapEntry *ORAMController::find_posmap_entry_by_block_id(int block_id) {
  for (auto& entry : position_map) {
    if (entry.second.block_id == block_id) {
      return &entry.second;
      }
  }
  return nullptr;
}

ORAMController::StashEntry *ORAMController::find_stash_entry_by_block_id(int block_id) {
  for (auto& entry : stash) {
    if (entry.second.block_id == block_id) {
      return &entry.second;
      }
  }
  return nullptr;
}

void ORAMController::remap(int block_id) {
  PosmapEntry *posmap_entry = find_posmap_entry_by_block_id(block_id);
  if(posmap_entry == nullptr) return;
  posmap_entry->leaf = oram_tree->get_random_leaf();
}

void ORAMController::tick() {
  m_clk++;
  
  // Integrity operations

  auto it = std::find_if(stash.begin(), stash.end(), [](const auto& pair) {
    return pair.second.integrity_checked == false;
  });
  //Now, it variable has a reference to the next block in which integrity has to be verified

  //FIXME: Perhaps the MEE is useless...a simple check_integrity function with clk latency could be the way
  //---
  if(current_transaction == nullptr) {
    if(!oram_transactions.empty()) {
      current_transaction = &oram_transactions.front();
    } else {
      return;
    }
  }
  
  if (current_transaction->phase == Phase::Reading) {
    // Send the pending READ request buffered before in send() function
    if(!current_transaction->pending_read.empty()) {
      Request load_request(current_transaction->pending_read.front(), Request::Type::Read);
      load_request.callback = [this](Request& req) {
        this->oram_read_callback(req);
      };
      if(send_to_controller(load_request)) {
        pathoram_read_requests++;
        current_transaction->pending_read.pop_front();
      }
    } else {
      // If all the read have been sent, then wait for the their completion (WaitingReadsDone)
      current_transaction->phase = Phase::WaitingReadsDone;
    }
  }
  else if (current_transaction->phase == Phase::WaitingReadsDone) {
    if(current_transaction->n_read_ack == required_acks) {
      // If all the read have been completed, call the original callback of the requested block
      // (to inform the CPU that its block has been READ).
      current_transaction->req.callback(current_transaction->req);
      remap(position_map.at(current_transaction->req.addr).block_id);
      current_transaction->phase = Phase::Writing;
    }
  }
  else if (current_transaction->phase == Phase::Writing) {
    // Finally, transition to Writing state to writeback all the dummy blocks stored in the stash.
    // Get the next dummy block (with block_id value of -1) in the stash
    auto it = std::find_if(stash.begin(), stash.end(), [](const auto& pair) {
      return pair.second.block_id == -1;
    });

    // If the iterator equals to the stash.end() it means all the dummy blocks have been written back to memory.
    // The following piece of code is suitable only for a '1 per time transaction'.
    //TODO: REFACTORING!
    if(it == stash.end()) {
      // The last block is to data block
      if(current_transaction->req.type_id == Request::Type::Write) {
        //If the original request is a Write, then Writeback the data block to memory
        PosmapEntry pe = position_map.at(current_transaction->req.addr);
        // Get it in write_it
        auto write_it = std::find_if(stash.begin(), stash.end(), [pe](const auto& pair) {
          return pair.second.block_id == pe.block_id;
        });
        if(!write_it->second.writebacked) {
          write_it->second.writeback_addr = oram_tree->insert_to_available_slot(pe.leaf, pe.block_id);
          write_it->second.writebacked = true;
        }
        
        //And writeback to memory
        Request write_request(write_it->second.writeback_addr, Request::Type::Write);
        if(send_to_controller(write_request)) {
          pathoram_write_requests++;
          stash.erase(write_it);
          current_transaction = nullptr;
          oram_transactions.pop_front();
        }
        return;
      }
      
      current_transaction = nullptr;
      oram_transactions.pop_front();
      return;
    }  
    
    // If a dummy block is found, writeback it to the DRAM Memory and remove it from stash
    Request write_request(it->first, Request::Type::Write);
    if(send_to_controller(write_request)) {
      pathoram_write_requests++;
      stash.erase(it);
    }
  }
}

bool ORAMController::send(Request req) {
  uint64_t leaf = position_map.at(req.addr).leaf;
  ORAMTransaction new_oram_transaction(Phase::Reading, 0, req, oram_tree->get_path_from_root(leaf));
  oram_transactions.push_back(new_oram_transaction);
  return true;
}

std::pair<std::unordered_map<Addr_t, ORAMController::PosmapEntry>::iterator, bool> ORAMController::posmap_allocate_entry(Addr_t addr, int block_id, uint64_t leaf) {
  return position_map.insert({addr, {block_id, leaf}});
}

uint64_t ORAMController::posmap_get_entry_leaf(Addr_t addr) {
    return position_map.at(addr).leaf;
}

int ORAMController::get_num_read_reqs() {
  return pathoram_read_requests;
}

int ORAMController::get_num_write_reqs() {
  return pathoram_write_requests;
}

int ORAMController::get_num_other_reqs() {
  return pathoram_other_requests;
}

void ORAMController::print_position_map() {
  printf("Position_map:\n");
  for (const auto& pair : position_map) {
    Addr_t addr = pair.first;
    const PosmapEntry& entry = pair.second;
    printf("Addr: %lu | Block ID: %d | Leaf: %lu\n", addr, entry.block_id, entry.leaf);
  }
}

void ORAMController::print_stash() {
  printf("Stash:\n");
  for (const auto& pair : stash) {
    Addr_t addr = pair.first;
    const StashEntry& entry = pair.second;
    printf("Addr: %lu | Block ID: %d\n", addr, entry.block_id);
  }
}

}   // namespace Ramulator