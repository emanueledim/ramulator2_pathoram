#ifndef I_MEE_H
#define I_MEE_H

#include "base/request.h"
#include "memory_system/impl/oram/components/interfaces/ioram_controller.h"
#include "memory_system/impl/oram/components/interfaces/iintegrity_controller.h"

namespace Ramulator {

class ORAMController;

class IMEE {

    public:
        IMEE() {};
        virtual ~IMEE() {};

        virtual bool send(Request& req) = 0;
        
        virtual void connect_oram_controller(IORAMController* oram_controller) = 0;

        virtual void connect_integrity_controller(IIntegrityController* integrity_controller) = 0;
};

}

#endif // I_MEE_H