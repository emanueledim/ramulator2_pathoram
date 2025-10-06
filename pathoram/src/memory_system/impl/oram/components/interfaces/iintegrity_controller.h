#ifndef I_INTEGRITY_CONTROLLER_H
#define I_INTEGRITY_CONTROLLER_H

#include "memory_system/impl/oram/components/inc/oram_tree_info.h"
#include "memory_system/impl/oram/components/interfaces/ioram_controller.h"

namespace Ramulator {

class IORAMController;

/**
 * @class IIntegrityController
 * @brief Interface for Integrity Controller class
 */
class IIntegrityController {
    
    public:
        IIntegrityController() {};
        virtual ~IIntegrityController() {};

        /**
         * @brief Enqueue a new integrity check request.
         */
        virtual void enqueue_block(Request& req) = 0;

        /**
         * @brief Connect the ORAM Controller.
         */
        virtual void connect_oram_controller(IORAMController* oram_controller) = 0;

        /**
         * @brief Attach the ORAM Tree Info to Integrity Controller with Dependency Injection.
         */
        virtual void attach_oram_info(const ORAMTreeInfo* oram_tree_info) = 0;

        /**
         * @brief Set the Integrity Controller's counters.
         */
        virtual void set_counters(std::map<std::string, size_t&>& counters) = 0;
};

}

#endif // I_INTEGRITY_CONTROLLER_H