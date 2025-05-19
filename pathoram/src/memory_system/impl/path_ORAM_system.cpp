#include <queue>
#include <unordered_map>
#include <cstdio>
#include <random>

#include "memory_system/memory_system.h"
#include "translation/translation.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"
#include "dram/dram.h"
#include "memory_system/impl/oram/oram_controller.h"
#include "memory_system/impl/oram/bucket_header.h"
#include "memory_system/impl/oram/oram_tree.h"

namespace Ramulator {

class PathORAMSystem final : public IMemorySystem, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IMemorySystem, PathORAMSystem, "PathORAM", "A PathORAM-based memory system.");

  private:
    ORAMController * oram_controller;
    ORAMTree *oram_tree;
    Addr_t max_paddr;

  protected:
    Clk_t m_clk = 0;
    IDRAM*  m_dram;
    IAddrMapper*  m_addr_mapper;
    std::vector<IDRAMController*> m_controllers;

  public:
    int pathoram_num_read_requests = 0;
    int pathoram_num_write_requests = 0;
    int pathoram_num_other_requests = 0;

    int s_num_read_requests = 0;
    int s_num_write_requests = 0;
    int s_num_other_requests = 0;


  public:
    void init() override {
      // Create device (a top-level node wrapping all channel nodes)
      m_dram = create_child_ifce<IDRAM>();
      m_addr_mapper = create_child_ifce<IAddrMapper>();

      int num_channels = m_dram->get_level_size("channel");   

      // Create memory controllers
      for (int i = 0; i < num_channels; i++) {
        IDRAMController* controller = create_child_ifce<IDRAMController>();
        controller->m_impl->set_id(fmt::format("Channel {}", i));
        controller->m_channel_id = i;
        m_controllers.push_back(controller);
      }

      m_clock_ratio = param<uint>("clock_ratio").required();

      register_stat(m_clk).name("memory_system_cycles");
      register_stat(s_num_read_requests).name("total_num_read_requests");
      register_stat(s_num_write_requests).name("total_num_write_requests");
      register_stat(s_num_other_requests).name("total_num_other_requests");

      //PathORAM
      register_stat(pathoram_num_read_requests).name("pathoram_num_read_requests");
      register_stat(pathoram_num_write_requests).name("pathoram_num_write_requests");
      register_stat(pathoram_num_other_requests).name("pathoram_num_other_requests");

      max_paddr   = param<Addr_t>("max_addr").desc("Max physical address of the memory system.").required();
      uint32_t block_size  = param<uint32_t>("block_size").desc("Size of a block in cache.").default_val(64);
      uint32_t z_blocks = param<uint32_t>("z_blocks").desc("Number of blocks in a bucket.").default_val(4);
      int arity = param<int>("arity").desc("Arity of ORAM Tree.").default_val(2);
      uint32_t stash_size = param<uint32_t>("stash_size").desc("Stash's size.").default_val(4096); //TODO: da utilizzare
      
      oram_tree = new ORAMTree(max_paddr, block_size, z_blocks, arity); //binary tree
      oram_controller = new ORAMController(oram_tree, m_addr_mapper, m_controllers);
    };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {}

    bool send(Request req) override {
      switch (req.type_id) {
        case Request::Type::Read: {
          s_num_read_requests++;
          break;
        }
        case Request::Type::Write: {
          printf("Received a write for %ld\n", req.addr);
          s_num_write_requests++;
          break;
        }
        default: {
          s_num_other_requests++;
          break;
        }
      }
      // This piece of code cares about init a path for the requested block in the Out of Band tree
      //FIXME: This has to be handled by ORAM Controller! Remove posmap_exists_entry() and stash_exists_entry()
      if(oram_controller->posmap_exists_entry(req.addr)) {
        if(oram_controller->stash_exists_entry(req.addr)) {
          req.callback(req);
          return true;
        }
        uint64_t leaf = oram_controller->posmap_get_entry_leaf(req.addr);
        oram_tree->init_path(leaf);
        return oram_controller->send(req);
      }

      uint64_t leaf = oram_tree->get_random_leaf();
      int block_id = oram_tree->get_block_id();
      // Allocate all the Bucket Header along the Oram Tree path,
      // assign the data block (it will be the requested block) and some dummy blocks
      oram_tree->init_path(leaf);
      Addr_t data_block_addr = oram_tree->init_data_block(leaf);
      oram_tree->init_dummy_blocks(leaf, 10);

      // Allocate the info about the requested block in the position map
      oram_controller->posmap_allocate_entry(req.addr, block_id, leaf);

      // Send and buffer the requested block in the ORAM Controller
      return oram_controller->send(req);
    };

    void tick() override {
      m_clk++;
      m_dram->tick();
      for (auto controller : m_controllers) {
        controller->tick();
      }
      oram_controller->tick();
      pathoram_num_read_requests = oram_controller->get_num_read_reqs(); 
      pathoram_num_write_requests = oram_controller->get_num_write_reqs(); 
      pathoram_num_other_requests = oram_controller->get_num_other_reqs(); 
    };

    float get_tCK() override {
      return m_dram->m_timing_vals("tCK_ps") / 1000.0f;
    }

    // const SpecDef& get_supported_requests() override {
    //   return m_dram->m_requests;
    // };
};
  
}   // namespace 

