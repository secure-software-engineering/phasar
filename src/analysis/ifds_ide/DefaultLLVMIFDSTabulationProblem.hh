/*
 * DefaultLLVMIFDSTabulationProblem.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTLLVMIFDSTABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_DEFAULTLLVMIFDSTABULATIONPROBLEM_HH_

#include "DefaultIFDSTabulationProblem.hh"
#include "ZeroValue.hh"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <type_traits>
#include "IFDSSpecialSummaries.hh"

//template <class D, class I>
//class DefaultLLVMIFDSTabulationProblem
//    : public DefaultIFDSTabulationProblem<const llvm::Instruction *, D,
//                                          const llvm::Function *, I> {
//public:
//  DefaultLLVMIFDSTabulationProblem(I icfg)
//      : DefaultIFDSTabulationProblem<const llvm::Instruction *, D,
//                                     const llvm::Function *, I>(icfg) {}
//  virtual ~DefaultLLVMIFDSTabulationProblem() = default;
//  virtual shared_ptr<FlowFunction<D>>
//  getSummaryFlowFunction(const llvm::Instruction *callStmt,
//                         const llvm::Function *destMthd) override {
//    SpecialSummaries<D> &Summaries = SpecialSummaries<D>::getInstance();
//    if (Summaries.containsSpecialSummary(destMthd)) {
//      return Summaries.getSpecialSummary(destMthd);
//    } else {
//      return nullptr;
//    }
//  }
//  virtual D createZeroValue() override {
//    static ZeroValue *zero = new ZeroValue;
//    return zero;
//  }
//};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTLLVMIFDSTABULATIONPROBLEM_HH_ */
