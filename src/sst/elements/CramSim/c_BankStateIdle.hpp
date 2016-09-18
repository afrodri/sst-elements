// Copyright 2015 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef C_BANKSTATEIDLE_HPP
#define C_BANKSTATEIDLE_HPP

// C++ includes
// #include <memory>
#include <list>

// CramSim includes
// #include "c_Bank.hpp"
#include "c_BankCommand.hpp"
#include "c_BankInfo.hpp"

namespace SST {
namespace n_Bank {

// forward declaration
class c_BankCommand;

class c_BankStateIdle: public c_BankState {

public:

	c_BankStateIdle(std::map<std::string, unsigned>* x_bankParams);
	~c_BankStateIdle();

	virtual void handleCommand(c_BankInfo* x_bank, c_BankCommand* x_bankCommandPtr);

	virtual void clockTic(c_BankInfo* x_bank);
	virtual void enter(c_BankInfo* x_bank, c_BankState* x_prevState, c_BankCommand* x_cmdPtr);
	virtual std::list<e_BankCommandType> getAllowedCommands();

	virtual bool isCommandAllowed(c_BankCommand* x_cmdPtr,
			c_BankInfo* x_bankPtr);

private:


	std::list<e_BankCommandType> m_allowedCommands;
	c_BankCommand* m_prevCommandPtr;
	c_BankCommand* m_receivedCommandPtr;
	unsigned m_timer;


};
}
}

#endif // C_BANKSTATEIDLE_HPP
