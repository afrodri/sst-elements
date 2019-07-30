// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_RVMA_EVENT
#define _H_EMBER_RVMA_EVENT

#include "emberevent.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberRvmaEvent : public EmberEvent {

  public:

    EmberRvmaEvent( RVMA::Interface& api, Output* output, int* retval = NULL ) :
        EmberEvent( output, retval ), m_api( api )
    {
        m_state = IssueCallbackPtr;
    }

  protected:

    RVMA::Interface&   m_api;

  private:
};


}
}

#endif
