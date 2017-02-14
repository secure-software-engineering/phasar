/*
 * DefaultLLVMIDETabulationProblem.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTLLVMIDETABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_DEFAULTLLVMIDETABULATIONPROBLEM_HH_

#include <type_traits>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include "DefaultIDETabulationProblem.hh"

template<class D, class V, class I>
class DefaultLLVMIDETabulationProblem : public DefaultIDETabulationProblem<llvm::Instruction,D,llvm::Function,V,I> {
	//static_assert(std::is_base_of<ICFG<llvm::Instruction,llvm::Function>,I>::value, "I requires an implementation of interface ICFG!");

public:
	virtual ~DefaultLLVMIDETabulationProblem() = default;
};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTLLVMIDETABULATIONPROBLEM_HH_ */
