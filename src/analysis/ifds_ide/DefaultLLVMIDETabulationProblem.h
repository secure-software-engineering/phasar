/*
 * DefaultLLVMIDETabulationProblem.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTLLVMIDETABULATIONPROBLEM_H_
#define ANALYSIS_IFDS_IDE_DEFAULTLLVMIDETABULATIONPROBLEM_H_

#include "DefaultIDETabulationProblem.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <type_traits>

template <class D, class V, class I>
class DefaultLLVMIDETabulationProblem
    : public DefaultIDETabulationProblem<const llvm::Instruction *, D,
                                         const llvm::Function *, V, I> {
  // static_assert(std::is_base_of<ICFG<llvm::Instruction,llvm::Function>,I>::value,
  // "I requires an implementation of interface ICFG!");

public:
  virtual ~DefaultLLVMIDETabulationProblem() = default;
};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTLLVMIDETABULATIONPROBLEM_HH_ */
