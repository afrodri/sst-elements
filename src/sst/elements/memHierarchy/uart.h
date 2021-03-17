// Copyright 2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2020 NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _UART_H
#define _UART_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>

#include "sst/elements/memHierarchy/memoryController.h"

namespace SST {
namespace MemHierarchy {

class UARTMessage : public SST::Event
{
public:
    UARTMessage() : SST::Event() { }
    uint32_t getData() {return data;}
    void setData(uint32_t d) {data = d;}

public:    
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & data;
    }

    ImplementSerializable(SST::MemHierarchy::UARTMessage);

protected:
    uint32_t data;
};
    
class UART : public MemController {
public:
    SST_ELI_REGISTER_COMPONENT(UART, "memHierarchy", "UART", SST_ELI_ELEMENT_VERSION(1,0,0),
            "UART component for sending and recieving", COMPONENT_CATEGORY_MEMORY)
    SST_ELI_DOCUMENT_PARAMS( MEMCONTROLLER_ELI_PARAMS )
    SST_ELI_DOCUMENT_PORTS( MEMCONTROLLER_ELI_PORTS,
                            {"other_UART",     "Connection to a second UART for communication. If no second UART, sends to stdout", {"memHierarchy.UARTMessage"} })    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( MEMCONTROLLER_ELI_SUBCOMPONENTSLOTS )

    UART(ComponentId_t id, Params &params);

    virtual void init(unsigned int); // user memcontroller
    virtual void setup();

    virtual void handleMemResponse( SST::Event::id_type id, uint32_t flags );

protected:
    UART();  // for serialization only
    ~UART() {}

    virtual void handleEvent( SST::Event* );
    virtual void handleMemEvent( MemEvent* );
    virtual void processInitEvent( MemEventInit* );
    void handleRead(MemEvent*);
    void handleWrite(MemEvent*);

    virtual bool clock( SST::Cycle_t );

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);
    //virtual void emergencyShutdown();

    // verbosity
    uint32_t verbose;
private:
    void handleUARTEvent(SST::Event *event);

    // where incoming data is stored
    deque<uint8_t> incomingBuffer;
    uint32_t tx_buffer;
    
    SST::Link *other_UART;
    
}; // end UART
        
}
}


#endif /*  _UART_H */
