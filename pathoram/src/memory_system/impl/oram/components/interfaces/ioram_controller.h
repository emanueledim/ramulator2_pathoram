#ifndef I_ORAM_CONTROLLER_H
#define I_ORAM_CONTROLLER_H

#include "memory_system/impl/oram/components/inc/oram_tree_info.h"
#include "memory_system/impl/oram/components/interfaces/iintegrity_controller.h"

namespace Ramulator {

/**
 * @class IORAMController
 * @brief Interface for ORAM Controller class
 */
class IORAMController {

    public:
        IORAMController() {};
        virtual ~IORAMController() {};

        /**
         * @brief Send to ORAM Controller a new memory Request.
         */
        virtual bool send(Request req) = 0;

        /**
         * @brief Connect the Integrity Controller.
         */
        virtual void connect_integrity_controller(IIntegrityController* integrity_controller) = 0;

        /**
         * @brief Set the integrity check for the requested block.
         */
        virtual void integrity_check(Addr_t addr) = 0;
        
        /**
         * @brief Attach the ORAM Tree Info to ORAM Controller with Dependency Injection.
         */
        virtual void attach_oram_info(const ORAMTreeInfo* oram_tree_info) = 0;

        /**
         * @brief Set the ORAM Controller's counters.
         */
        virtual void set_counters(std::map<std::string, size_t&>& counters) = 0;
};

}

#endif // I_ORAM_CONTROLLER_H