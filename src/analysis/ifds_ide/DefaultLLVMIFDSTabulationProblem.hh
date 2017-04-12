/*
 * DefaultLLVMIFDSTabulationProblem.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTLLVMIFDSTABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_DEFAULTLLVMIFDSTABULATIONPROBLEM_HH_

#include <type_traits>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Function.h>
#include "DefaultIFDSTabulationProblem.hh"

template<class D, class I>
class DefaultLLVMIFDSTabulationProblem : public DefaultIFDSTabulationProblem<const llvm::Instruction*,
																																						 D,
																																						 const llvm::Function*,
																																						 I> {
//static_assert(std::is_base_of<ICFG<llvm::Instruction,llvm::Function>, I>::value, "I requires an implementation of interface ICFG!");

public:
	virtual ~DefaultLLVMIFDSTabulationProblem() = default;
};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTLLVMIFDSTABULATIONPROBLEM_HH_ */
