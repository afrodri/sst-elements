
#include <sst_config.h>
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

HybridSimMemory::HybridSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string hybridIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if(hybridIniFilename == NO_STRING_DEFINED)
        _abort(MemController, "XML must define a 'system_ini' file parameter\n");

    memSystem = HybridSim::getMemorySystemInstance( 1, hybridIniFilename);

    typedef HybridSim::Callback <HybridSimMemory, void, uint, uint64_t, uint64_t> hybridsim_callback_t;
    HybridSim::TransactionCompleteCB *read_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    HybridSim::TransactionCompleteCB *write_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    memSystem->RegisterCallbacks(read_cb, write_cb);
}



bool HybridSimMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;

    bool ok = memSystem->WillAcceptTransaction();
    if(!ok) return false;
    ok = memSystem->addTransaction(req->isWrite_, addr);
    if(!ok) return false;  // This *SHOULD* always be ok
    ctrl->dbg.debug(_L10_, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    dramReqs[addr].push_back(req);
    return true;
}



void HybridSimMemory::clock(){
    memSystem->update();
}



void HybridSimMemory::finish(){
    memSystem->printLogfile();
}



void HybridSimMemory::hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    std::deque<MemController::DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(_L10_, "Memory Request for %"PRIx64" Finished [%zu reqs]\n", addr, reqs.size());
    assert(reqs.size());
    MemController::DRAMReq *req = reqs.front();
    reqs.pop_front();
    if(reqs.size() == 0)
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}
