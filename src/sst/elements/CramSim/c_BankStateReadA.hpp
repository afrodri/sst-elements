// Copyright 2009-2018 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Copyright 2016 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef C_BANKSTATEREADA_HPP
#define C_BANKSTATEREADA_HPP

#include <memory>
#include <list>

#include "c_BankState.hpp"
#include "c_BankInfo.hpp"
#include "c_BankCommand.hpp"
#include "c_BankStatePrecharge.hpp"

namespace SST {
namespace n_Bank {

//class c_BankStatePrecharge;

class c_BankStateReadA: public c_BankState {

public:

	c_BankStateReadA(std::map<std::string, unsigned>* x_bankParams);
	~c_BankStateReadA();

	virtual void handleCommand(c_BankInfo* x_bank, c_BankCommand* x_bankCommandPtr, SimTime_t x_cycle);

	virtual void clockTic(c_BankInfo* x_bank, SimTime_t x_cycle);

	virtual void enter(c_BankInfo* x_bank, c_BankState* x_prevState,
			c_BankCommand* x_cmdPtr, SimTime_t x_cycle);

	virtual std::list<e_BankCommandType> getAllowedCommands();

	virtual bool isCommandAllowed(c_BankCommand* x_cmdPtr,
			c_BankInfo* x_bankPtr);


private:
	SimTime_t m_timerEnter; //<! counts down to 0. when 0, changes state to ACTIVE automatically. is reset to ?? at state entry.
	SimTime_t m_timerExit; //<! counts down to 0 and models the exit to PRE time
	std::list<e_BankCommandType> m_allowedCommands;
	c_BankCommand* m_prevCommandPtr;
	c_BankState* m_nextStatePtr;
};

}
}
#endif // C_BANKSTATEREADA_HPP
