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

#include <sst_config.h>
#include "uart.h"

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"

using namespace SST;
using namespace SST::MemHierarchy;
    

// Debug macros
#ifdef __SST_DEBUG_OUTPUT__
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#define Debug(level, fmt, ... )
#endif

UART::UART(ComponentId_t id, Params &params) : MemController(id, params) {
    // initilize other_uart link
    other_UART = configureLink( "other_UART", "1ns",
                                new Event::Handler<UART>(this, &UART::handleUARTEvent));
}

void UART::init(unsigned int phase) {
    MemController::init(phase); // call super
}

void UART::setup() {
    MemController::setup(); // call super

    // turn off backend
    memBackendConvertor_->turnClockOff();
}

void UART::handleMemResponse( SST::Event::id_type id, uint32_t flags ) {
    // this shouldn't happen, as we don't use the backend
    out.fatal(CALL_INFO,-1,"UART got respsonse from backend\n");
}

void UART::handleEvent(SST::Event * event) {
    MemEventBase *meb = static_cast<MemEventBase*>(event);

    if (is_debug_event(meb)) {
        Debug(_L3_, "\n%" PRIu64 " (%s) Received: %s\n", getCurrentSimTimeNano(), getName().c_str(), meb->getVerboseString().c_str());
    }

    Command cmd = meb->getCmd();

    if (cmd == Command::CustomReq) {
        out.fatal(CALL_INFO, -1, "UART does not support custom events\n");
        return;
    }

    MemEvent * ev = static_cast<MemEvent*>(meb);

    // sanity check
    if (!region_.contains(ev->getBaseAddr())) {
        out.fatal(CALL_INFO, -1, "%s, Error: Received an event with a base address that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString().c_str());
    }
    bool noncacheable = ev->queryFlag(MemEvent::F_NONCACHEABLE);
    assert(noncacheable);
    
    // ???
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    // Notify our listeners that we have received an event
    notifyListeners( ev );

    switch (cmd) {
        case Command::PutM:
            ev->setFlag(MemEvent::F_NORESPONSE);
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            handleMemEvent( ev );
            break;

        case Command::FlushLine:
        case Command::FlushLineInv:
            out.fatal(CALL_INFO, -1, "UART does not support cache flush events\n");
            break;

        case Command::PutS:
        case Command::PutE:
            delete ev;
            break;
        default:
            out.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[(int)cmd]);
    }
}

// handle the actual memory event and response
void UART::handleMemEvent( MemEvent *ev ) {
    // handle & respond

    bool noncacheable  = ev->queryFlag(MemEvent::F_NONCACHEABLE);

    /* Write data. */
    if (ev->getCmd() == Command::PutM || (ev->getCmd() == Command::GetX && noncacheable)) {
        handleWrite(ev);
    }

    if (ev->queryFlag(MemEvent::F_NORESPONSE)) {
        delete ev;
        return;
    }

    MemEvent * resp = ev->makeResponse();

    /* Read order matches execute order so that mis-ordering at backend can result in bad data */
    if (resp->getCmd() == Command::GetSResp || (resp->getCmd() == Command::GetXResp && !noncacheable)) {
        handleRead(resp);
        if (!noncacheable) resp->setCmd(Command::GetXResp);
    }

    resp->setFlags(ev->getFlags());

    if (ev->isAddrGlobal()) {
        resp->setBaseAddr(translateToGlobal(ev->getBaseAddr()));
        resp->setAddr(translateToGlobal(ev->getAddr()));
    }

    link_->send( resp );
    delete ev;
}

void UART::handleWrite(MemEvent *event) {
    printf("Handle Write\n");
    /* Noncacheable events occur on byte addresses, others on line addresses */
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr addr = noncacheable ? event->getAddr() : event->getBaseAddr();

    assert((event->getCmd() == Command::Put) ||
           (noncacheable && event->getCmd() == Command::GetX));

    // ignore higher bits in address
    if ((addr & 0xfff) == 0) { // tx_send
        //backing_->set(addr, event->getSize(), event->getPayload());
        if (other_UART) {
            // send to other UART from buffer
            UARTMessage *msg = new UARTMessage();
            msg->setData(tx_buffer);
            printf("UART sending: 0x%08" PRIx32 "\n", msg->getData());
            other_UART->send(msg);
        } else {
            // print buffer to stdout
            printf("UART output: 0x%08" PRIx32 "\n", tx_buffer);
        }
    } else {
        // add to tx buffer
        tx_buffer = 0;
        size_t size = event->getSize();
        int dataOffset = size-1;
        vector<uint8_t> &data = event->getPayload();
        printf("uart adding to tx-buffer (sz %ld addr %llx):", size, addr);
        while(dataOffset >= 0) {
            // add to buffer
            printf("%x ", data[dataOffset]);
            tx_buffer <<= 8;
            tx_buffer += data[dataOffset];
            dataOffset--;
        }
        printf("\n");
    }
}

void UART::handleRead(MemEvent *resp) {
    bool noncacheable = resp->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? resp->getAddr() : resp->getBaseAddr();

    vector<uint8_t> payload;
    payload.resize(resp->getSize(), 0);

    printf("UART Read from %llx %d bytes\n", localAddr, resp->getSize());

    // we're a simple, humble 32-bit UART, not one of those fancy
    // 64-bit'ers
    const int maxRespSize = 4;
    assert(resp->getSize() == maxRespSize);

    // ignore higher order bits in address
    if ((localAddr & 0xfff) == 0x4) {
        // return if we have data
        if (!incomingBuffer.empty()) {
            payload[0] = 1;
        }
        printf("UART ready status: %x\n", payload[0]);
    } else { // read from buffer
        if (!incomingBuffer.empty()) { // if nothing to read, return 0s
            uint8_t inData = 0;
            int dataOffset = 0;
            printf("UART Read from buffer:");
            while((dataOffset < maxRespSize) && !incomingBuffer.empty()) {
                inData = incomingBuffer.front();
                incomingBuffer.pop_front();
                payload[dataOffset] = inData;
                printf("0x%x ", inData);
                dataOffset++;
            }
            printf("\n");
        } 
    }
    resp->setPayload(payload);
}

void UART::processInitEvent( MemEventInit* me ) {
#if 0
    // really shoudn't get any GetX events
    if (Command::GetX == me->getCmd()) {
        out.fatal(CALL_INFO,-1,"UART recieved GetX during Init. This may indicate a binary that is trying to load into a reserved address used by the UART. addr=%llx/%llx\n", me->getAddr(), translateToLocal(me->getAddr()));
    } else if (Command::NULLCMD == me->getCmd()) {
        if (is_debug_event(me)) { Debug(_L9_, "Memory (%s) received init event: %s\n", getName().c_str(), me->getVerboseString().c_str()); }
    } else {
        out.debug(_L10_,"Memory received unexpected Init Command: %d\n", (int)me->getCmd());
    }

    delete me;
#endif
        MemController::processInitEvent(me);
}

bool UART::clock( SST::Cycle_t ) {
    return false;
}

void UART::printStatus(Output &out) 
{
    MemController::printStatus(out); // call super
}

// handle UART-to-UART events
void UART::handleUARTEvent(SST::Event * event) {
    UARTMessage *msg = dynamic_cast<UARTMessage*>(event);
    assert(msg);

    uint32_t inData = msg->getData();
    printf("UART got %08x from other UART\n", inData);
    int dataOffset = 0;
    while(dataOffset < sizeof(inData)) {
        uint8_t d = inData & 0xff;
        inData >>= 8;
        incomingBuffer.push_back(d);
        dataOffset++;
    }
    
    delete event;
}
