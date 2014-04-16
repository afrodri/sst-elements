// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_VANADIS_HWCONTEXT
#define _H_SST_VANADIS_HWCONTEXT


#include "regset.h"
#include "dispatch.h"

namespace SST {
namespace Vanadis {

class VanadisHWContext {

	protected:
		VanadisRegisterSet* regset;
		VanadisDispatchEngine* dispatcher;

		uint64_t pc;
		uint32_t threadID;
		bool inHalt;

	public:
		VanadisHWContext(uint32_t thrID, VanadisRegisterSet* regset, VanadisDispatchEngine* dispatch);
		void dispatch();
		bool inHaltState();
		uint32_t getThreadID();


};

}
}

#endif
