/*
 * IFDSTabulationProblemPlugin.hh
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMPLUGIN_HH_
#define SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMPLUGIN_HH_

#include <memory>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <string>
#include "../../../lib/LLVMShorthands.hh"
#include <llvm/IR/Function.h>
#include "../../icfg/LLVMBasedICFG.hh"
#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
using namespace std;

class IFDSTabulationProblemPlugin : public DefaultIFDSTabulationProblem<const llvm::Instruction*,
																																				const llvm::Value*,
																																				const llvm::Function*,
																																				LLVMBasedICFG&> {
public:
	IFDSTabulationProblemPlugin(LLVMBasedICFG& ICFG) : DefaultIFDSTabulationProblem<const llvm::Instruction*,
																																									const llvm::Value*,
																																									const llvm::Function*,
																																									LLVMBasedICFG&>(ICFG) {
		DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
	}

	const llvm::Value *createZeroValue() {
		// cout << "IFDSSolverTest::createZeroValue()\n";
	  // // create a special value to represent the zero value!
	  // static ZeroValue *zero = new ZeroValue;
		// return zero;
		return nullptr;
	}
};

extern "C" unique_ptr<IFDSTabulationProblemPlugin> createIFDSTabulationProblemPlugin(LLVMBasedICFG& I);

#endif /* SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMPLUGIN_HH_ */
