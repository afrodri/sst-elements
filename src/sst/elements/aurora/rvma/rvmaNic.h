// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_AURORA_RVMA_NIC_H
#define COMPONENTS_AURORA_RVMA_NIC_H

#include <queue>
#include "include/nicSubComponent.h"
#include "rvmaNicCmds.h"
#include "include/networkPkt.h"
#include "sst/elements/hermes/rvma.h"

namespace SST {
namespace Aurora {
namespace RVMA {

class RvmaNicSubComponent : public Aurora::NicSubComponent {

  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        RvmaNicSubComponent,
        "aurora",
        "rvmaNic",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Aurora::RVMA::RvmaNicSubComponent 
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"verboseLevel","Sets the level of debug verbosity",""},
        {"verboseMask","Sets the debug mask",""},
    )

    RvmaNicSubComponent( Component* owner, Params& params ) : NicSubComponent(owner) {} 
    RvmaNicSubComponent( ComponentId_t id, Params& params );
	void setup();

    void setRecvNotify( std::function<void()> func ) { m_recvNotify = func; }
    void setSendNotify( std::function<void()> func ) { m_sendNotify = func; }

	void handleEvent( int core, Event* event );
    bool clockHandler( Cycle_t );

	struct Buffer {
		Buffer ( Hermes::MemAddr addr, size_t size, Hermes::RVMA::Completion* completion) : addr(addr), size(size), completion(completion), nBytes(0), filled(false) {}
		void copy( void* ptr, size_t offset, size_t length ) {
			assert( offset + length <= size );
			if ( addr.getBacking() ) {
				memcpy( addr.getBacking(offset), ptr, length);
			}
			nBytes += length;
		}
		Hermes::MemAddr addr;
		size_t size;
		Hermes::RVMA::Completion* completion;
		size_t nBytes;
		bool filled;
	};

	Link* m_selfLink;

	class SendEntry {
	  public:
	  	SendEntry( int srcCore, PutCmd* cmd ) : srcCore(srcCore), cmd(cmd), currentOffset(0), completedDMAs(0)  {
			totalDMAs = cmd->size / RvmaNicSubComponent::getPktSize();
			if ( cmd->size % RvmaNicSubComponent::getPktSize() ) { 
				++totalDMAs;
			}
		} 
		~SendEntry() { delete cmd; }

		void incCompletedDMAs() { ++completedDMAs; }
		void decRemainingBytes( int num ) { currentOffset += num; }
		int remainingBytes() { return cmd->size - currentOffset; }
		size_t offset() { return currentOffset + cmd->offset; }
		int getDestNode() { return cmd->proc.node; }
		bool done() { return totalDMAs == completedDMAs; }
		uint64_t virtAddr() { return cmd->virtAddr; }
		size_t length() { return cmd->size; }
		int destPid() { return cmd->proc.pid; }
		void* dataAddr() { return cmd->srcAddr.getBacking(); }
		int getSrcCore() { return srcCore; }
	  	PutCmd& getCmd() { return *cmd; }

	  private:
		int totalDMAs;
		int completedDMAs;
		int     srcCore;
	  	PutCmd* cmd;
		size_t currentOffset;
	};

	class SelfEvent : public Event {
	  public:
		SelfEvent( int slot, SendEntry* entry ) : Event(), slot(slot), entry(entry) {}
		int slot;
		SendEntry* entry;

		NotSerializable(SelfEvent)
	};


	class DMAslot {
		enum { Idle, Wait, Ready } state;
	  public:
		DMAslot() : state(Idle) {}	

		void init( NetworkPkt* pkt, int destNode ) {
			m_pkt = pkt;
			m_destNode = destNode;	
			state = Wait;
		}

		NetworkPkt* pkt() { return m_pkt; }

		bool ready() { return state == Ready; }
		int getDestNode() { return m_destNode; }
		void setIdle() { state = Idle; } 
		void setReady() { state = Ready; }

	  private:
		NetworkPkt* 	m_pkt;
		int 			m_destNode;
	};

	std::vector< DMAslot > m_dmaSlots;
	int m_firstActiveDMAslot;
	int m_firstAvailDMAslot;
	int m_activeDMAslots;

	class Window {
	  public:
		Window() : m_active(false) {}
		void setActive( Hermes::RVMA::VirtAddr addr, size_t threshold, Hermes::RVMA::EpochType type) {
			m_active = true; 
			m_addr = addr;
			m_threshold = threshold;
			m_epochType = type;
			m_count = 0;
		}
		bool isActive() { return m_active; }

		void close() {
			while ( ! m_usedBuffers.empty() ) {
				delete m_usedBuffers.front();
				m_usedBuffers.pop_front();
			}
			while ( ! m_availBuffers.empty() ) {
				delete m_availBuffers.front();
				m_availBuffers.pop_front();
			}
			m_active = false;
		}

		Buffer* recv( size_t offset, void* ptr, size_t nbytes ) {

			if ( m_availBuffers.empty() ) {
				return NULL;
			}

			Buffer* buffer = m_availBuffers.front();

			buffer->copy( ptr, offset, nbytes );
		
			if ( m_epochType == Hermes::RVMA::Op ) {
				++m_count;
			} else {
				m_count += nbytes;
			}

			printf("count=%zu threshold=%zu\n",m_count,m_threshold );
			if ( m_count == m_threshold ) {
				endEpoch( buffer );
			}
			return buffer;
		}

		void endEpoch( Buffer* buffer ) {
			buffer->filled = true;
			buffer->completion->count = m_count;	
			buffer->completion->addr = buffer->addr;	
			m_usedBuffers.push_back( buffer );
			m_availBuffers.pop_front();
			m_count = 0;
		}

		int getEpoch(int* epoch ) { 
			if ( totalBuffers() == 0 ) {
				return -1;
			}
			*epoch = m_usedBuffers.size();
			return 0; 
		}

		int incEpoch() { 
			if ( m_availBuffers.empty() ) {
                return -1;
            }
			endEpoch( m_availBuffers.front() ); 
			return 0; 
		}

		void getBufPtrs( std::vector<Hermes::RVMA::Completion>& vec, int max ) {
			std::deque<Buffer*>::iterator iter = m_usedBuffers.begin();
			for ( int i = 0; iter != m_usedBuffers.end() &&  i < max; i++, ++iter ) {
				vec.push_back( *(*iter)->completion );
			}	
		}
		int totalBuffers() { return m_availBuffers.size() + m_usedBuffers.size(); }

		int postBuffer( Hermes::MemAddr addr, size_t size, Hermes::RVMA::Completion* completion ) {
			std::deque<Buffer*>::iterator iter;

			for ( iter = m_usedBuffers.begin(); iter != m_usedBuffers.end(); ++iter ) {
				if ( (*iter)->addr.getSimVAddr() == addr.getSimVAddr() ) {
					m_availBuffers.push_back( *iter );
					m_usedBuffers.erase(iter);
					return 0;
				}
			}
			for ( iter = m_availBuffers.begin(); iter != m_availBuffers.end(); ++iter ) {
				if ( (*iter)->addr.getSimVAddr() == addr.getSimVAddr() ) {
					return -1;
				}
			}
			m_availBuffers.push_back( new Buffer( addr, size, completion) );
			return 0;
		}
		Hermes::RVMA::VirtAddr getVirtAddr() { return m_addr; }

		bool checkVirtAddr( Hermes::RVMA::VirtAddr addr ) {
			return ( addr == m_addr );
			//return ( addr >= m_addr && addr < m_addr + m_threshold );
		}

	  private:
		bool m_active;
		Hermes::RVMA::VirtAddr m_addr;
	   	size_t m_threshold;
		size_t m_count;
		Hermes::RVMA::EpochType m_epochType;
		std::deque<Buffer*> m_availBuffers;
		std::deque<Buffer*> m_usedBuffers;
	};

    bool recvNotify( int vc ) {
        m_dbg.debug(CALL_INFO,1,2,"\n");
        return false;
    }

    bool sendNotify( int vc ) {
        m_dbg.debug(CALL_INFO,1,2,"\n");
        return false;
    }

  private:

	void processRecv() {
		Interfaces::SimpleNetwork::Request* req = getNetworkLink().recv( m_vc );
    	if ( req ) {
        	NetworkPkt* pkt = static_cast<NetworkPkt*>(req->takePayload());
			processRVMA(pkt);
			delete req;
		}
	}
	void processRVMA( NetworkPkt* );
	void setNumCores( int num );
	void initWindow( int coreNum, Event* event );
	void closeWindow( int coreNum, Event* event );

	void incEpoch( int coreNum, Event* event );
	void getEpoch( int coreNum, Event* event );
	void getBufPtrs( int coreNum, Event* event );
	void postBuffer( int coreNum, Event* event );
	void put( int coreNum, Event* event );
	void mwait( int coreNum, Event* event );


	void handleSelfEvent( Event* );
	void processSendQ( Cycle_t cycle );
	void processDMAslots();

	struct Core {
		Core() : m_mwaitCmd(NULL) {}
		std::vector<Window> m_windowTbl;
		MwaitCmd*   		m_mwaitCmd;
		std::deque< Buffer* > m_completed;

		void saveCompleted( Buffer* buffer ) {
			m_completed.push_back(buffer);
		}

		bool checkCompleted( MwaitCmd* cmd ) {
			std::deque< Buffer* >::iterator iter = m_completed.begin();
			for ( ; iter != m_completed.end(); ++iter ) {
				if ( cmd->completion == (*iter)->completion ) {
					m_completed.erase(iter);
					return true;
				}
			}
			return false;
		}

		int findWindow( Hermes::RVMA::VirtAddr virtAddr ) {
			for ( int i = 0; i < m_windowTbl.size(); i++ ) {
				if ( m_windowTbl[i].isActive() ) {
					if ( m_windowTbl[i].checkVirtAddr( virtAddr ) ) {
						return i;
					}
				}	
			}
			return -1;
		}

	};

	std::vector<Core>   m_coreTbl;
	int m_maxBuffers;
	int m_maxNumWindows;

   	bool m_clocking;
    UnitAlgebra                 m_clockRate;
    Clock::Handler<RvmaNicSubComponent>*        m_clockHandler;
    TimeConverter* m_timeConverter;

	typedef void (RvmaNicSubComponent::*MemFuncPtr)( int, Event* );

	std::vector<MemFuncPtr> m_cmdFuncTbl;

	static const char *m_cmdName[];
	Output m_dbg;
	std::queue< SendEntry* > m_sendQ;

	int m_vc;

    std::function<void()> m_recvNotify;
    std::function<void()> m_sendNotify;
};

}
}
}
#endif