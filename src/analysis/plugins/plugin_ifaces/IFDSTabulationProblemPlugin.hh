/*
 * IFDSTabulationProblemPlugin.hh
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMPLUGIN_HH_
#define SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMPLUGIN_HH_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include "../../../lib/LLVMShorthands.hh"
#include "../../icfg/LLVMBasedICFG.hh"
#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
#include "../../ifds_ide/ZeroValue.hh"
using namespace std;

class IFDSTabulationProblemPlugin
    : public DefaultIFDSTabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, LLVMBasedICFG &> {
 public:
  IFDSTabulationProblemPlugin(LLVMBasedICFG &ICFG)
      : DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                     const llvm::Value *,
                                     const llvm::Function *, LLVMBasedICFG &>(
            ICFG) {
    DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
	}
	~IFDSTabulationProblemPlugin() = default;

  const llvm::Value *createZeroValue() override {
    // create a special value to represent the zero value!
    static ZeroValue *zero = new ZeroValue;
    return zero;
  }

  bool isZeroValue(const llvm::Value *d) const override {
    return isLLVMZeroValue(d);
  }

  string D_to_string(const llvm::Value *d) override {
    return llvmIRToString(d);
  }
};

extern "C" unique_ptr<IFDSTabulationProblemPlugin>
createIFDSTabulationProblemPlugin(LLVMBasedICFG &I);

#endif /* SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMPLUGIN_HH_ */
