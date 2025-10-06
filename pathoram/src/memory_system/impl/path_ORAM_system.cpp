#include <string>
#include <map>

#include "memory_system/memory_system.h"
#include "translation/translation.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"
#include "dram/dram.h"

#include "memory_system/impl/oram/components/interfaces/ioram_controller.h"
#include "memory_system/impl/oram/components/interfaces/iintegrity_controller.h"

#include "memory_system/impl/oram/oram_controller.h"
#include "memory_system/impl/oram/components/inc/integrity_controller.h"

#include "memory_system/impl/oram/components/inc/oram_tree_info.h"

#define LOG_REQS 0

namespace Ramulator {

class PathORAMSystem final : public IMemorySystem, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IMemorySystem, PathORAMSystem, "PathORAM", "A PathORAM-based memory system.");

  private:
    IORAMController* oram_controller;
    IIntegrityController* integrity_controller;
    ORAMTreeInfo* oram_tree_info;

  protected:
    Clk_t m_clk = 0;
    IDRAM* m_dram;
    IAddrMapper* m_addr_mapper;
    std::vector<IDRAMController*> m_controllers;

  public:
    std::map<std::string, size_t&> pathoram_counters;

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
      Addr_t base_address_tree = param<Addr_t>("base_address_tree").desc("Base address of the ORAM Tree in DRAM memory.").required();
      Addr_t length_tree = param<Addr_t>("length_tree").desc("Length of ORAM Tree in DRAM memory.").required();
      int block_size  = param<uint32_t>("block_size").desc("Size of a block in Bytes.").default_val(64);
      int z_blocks = param<uint32_t>("z_blocks").desc("Number of blocks in a bucket.").default_val(4);
      int arity = param<int>("arity").desc("Arity of ORAM Tree.").default_val(2);
      int stash_size = param<uint32_t>("stash_size").desc("Stash's max capacity.").default_val(8192);
      Clk_t encrypt_delay = param<uint>("encrypt_delay").desc("Number of clock cycles to encrypt a block.").default_val(0);
      Clk_t decrypt_delay = param<uint>("decrypt_delay").desc("Number of clock cycles to decrypt a block.").default_val(0);
      int hash_delay = param<int>("hash_delay").desc("Number of clock cycles to calculate the hash in Integrity Checker component.").default_val(0);

      oram_tree_info = new ORAMTreeInfo(base_address_tree, length_tree, block_size, z_blocks, arity);
      oram_controller = new ORAMController(stash_size, encrypt_delay, decrypt_delay, m_addr_mapper, m_controllers);
      integrity_controller = new IntegrityController(hash_delay);

      oram_controller->set_counters(pathoram_counters);
      integrity_controller->set_counters(pathoram_counters);
      
      oram_controller->attach_oram_info(oram_tree_info);
      integrity_controller->attach_oram_info(oram_tree_info);

      oram_controller->connect_integrity_controller(integrity_controller);
      integrity_controller->connect_oram_controller(oram_controller);

      for(auto e : pathoram_counters) {
        register_stat(e.second).name(e.first);
      }

      char filename[256];
      std::sprintf(filename, "stash_occupancy_%lu_%d_%d_%d_%d_%lu_%d_%d.csv", length_tree, block_size, z_blocks, arity, stash_size, encrypt_delay, hash_delay, num_channels);
      static_cast<ORAMController*>(oram_controller)->outdata.open(filename, std::ios::app);
    };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {}

    bool send(Request req) override {
      // Send and buffer the requested block in the ORAM Controller
      bool is_success = oram_controller->send(req);

      if(is_success) {
        
        #if LOG_REQS
        //type_id is 0 for read, 1 for write
        std::cout<< "Received type_id "<< req.type_id <<" request for "<< req.addr << std::endl;
        #endif

        switch (req.type_id) {
          case Request::Type::Read: {
            s_num_read_requests++;
            break;
          }
          case Request::Type::Write: {
            s_num_write_requests++;
            break;
          }
          default: {
            s_num_other_requests++;
            break;
          }
        }
      }
      return is_success;
    };

    void tick() override {
      m_clk++;
      m_dram->tick();
      for (auto controller : m_controllers) {
        controller->tick();
      }
      static_cast<IntegrityController*>(integrity_controller)->tick();
      static_cast<ORAMController*>(oram_controller)->tick();
    };

    float get_tCK() override {
      return m_dram->m_timing_vals("tCK_ps") / 1000.0f;
    }

    // const SpecDef& get_supported_requests() override {
    //   return m_dram->m_requests;
    // };

    ~PathORAMSystem() {
        delete oram_controller;
        delete integrity_controller;
        delete oram_tree_info;
    }
};
  
}   // namespace 

