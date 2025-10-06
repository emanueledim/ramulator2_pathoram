#ifndef MEE_H
#define MEE_H

#include <queue>

#include "base/base.h"
#include "base/request.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"
#include "memory_system/impl/oram/components/interfaces/imee.h"
#include "memory_system/impl/oram/oram_controller.h"

namespace Ramulator {

class MEE : public IMEE, Clocked<MEE> {

    struct CurrentComputation{
        Clk_t finish_time = 0;
    };

    enum class EncryptionState {Idle, Encrypting};
    enum class DecryptionState {Idle, Decrypting};

    private:
        // Ramulator components
        IAddrMapper* m_addr_mapper;
        std::vector<IDRAMController*> m_controllers;
        ORAMController* oram_controller;

        std::queue<Request> pending_wb_blocks;
        std::queue<Request> ready_wb_blocks;
        std::queue<Request> pending_rd_blocks;
        std::queue<Request> ready_rd_blocks;

        CurrentComputation current_encryption;
        CurrentComputation current_decryption;

        Clk_t encrypt_delay;
        Clk_t decrypt_delay;

        EncryptionState encryption_state;
        DecryptionState decryption_state;

        bool send_to_dram_controller(Request& req) {
            m_addr_mapper->apply(req);
            int channel_id = req.addr_vec[0];
            return m_controllers[channel_id]->send(req);
        }

        void mee_callback(Request& req) {
            printf("Received block from dram...\n");
            pending_rd_blocks.push(req);
        }

    public:
        void setORAMController(ORAMController* oram_controller) override {
            this->oram_controller = oram_controller;
        }

        MEE(Clk_t encrypt_delay, Clk_t decrypt_delay, IAddrMapper* m_addr_mapper, std::vector<IDRAMController*> m_controllers) 
            : encrypt_delay(encrypt_delay), decrypt_delay(decrypt_delay) {
            this->m_addr_mapper = m_addr_mapper;
            this->m_controllers = m_controllers;
        }

        void tick() override {
            m_clk++;
            
            // Send the next ready request to the DRAM 
            if(!ready_wb_blocks.empty()) {
                Request& next_req = ready_wb_blocks.front();
                if(send_to_dram_controller(next_req)) {
                    ready_wb_blocks.pop();
                }
            }

            if(!ready_rd_blocks.empty()) {
                Request& next_req = ready_rd_blocks.front();
                getchar();
                printf("Send to oram controller\n");
                //oram_controller->oram_read_callback(next_req);
                //FIXME: non può distinguere gli header
                //TODO: send to integrity checker
                ready_rd_blocks.pop();
            }

            if(encryption_state == EncryptionState::Idle) {
                if(!pending_wb_blocks.empty()) {
                    current_encryption.finish_time = m_clk + encrypt_delay;
                    encryption_state = EncryptionState::Encrypting;
                }
            }
            else if(encryption_state == EncryptionState::Encrypting) {
                if(m_clk >= current_encryption.finish_time) {
                    ready_wb_blocks.push(pending_wb_blocks.front());
                    pending_wb_blocks.pop();
                    encryption_state = EncryptionState::Idle;
                }
            }
            
            if(decryption_state == DecryptionState::Idle) {
                if(!pending_rd_blocks.empty()) {
                    printf("Decrypting...\n");
                    current_decryption.finish_time = m_clk + decrypt_delay;
                    decryption_state = DecryptionState::Decrypting;
                }
            }
            else if(decryption_state == DecryptionState::Decrypting) {
                if(m_clk >= current_decryption.finish_time) {
                    printf("Decryption finish...\n");
                    ready_rd_blocks.push(pending_rd_blocks.front());
                    pending_rd_blocks.pop();
                    decryption_state = DecryptionState::Idle;
                }
            }
        }

        bool send(Request& req) override {
            switch (req.type_id) {
                case Request::Type::Read: {
                    req.callback = [this](Request& req) {
                        this->mee_callback(req);
                    };
                    send_to_dram_controller(req);
                    break;
                }
                case Request::Type::Write: {
                    pending_wb_blocks.push(req);
                    break;
                }
                default: {
                    break;
                }
            }
            return true;
        }
};

}

#endif // MEE_H