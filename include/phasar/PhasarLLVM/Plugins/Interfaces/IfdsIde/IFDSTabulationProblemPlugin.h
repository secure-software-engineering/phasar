/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSTabulationProblemPlugin.h
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IFDSTABULATIONPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IFDSTABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class IFDSTabulationProblemPlugin
    : public DefaultIFDSTabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, LLVMBasedICFG &> {
protected:
  std::vector<std::string> EntryPoints;

public:
  IFDSTabulationProblemPlugin(LLVMBasedICFG &ICFG,
                              std::vector<std::string> EntryPoints = {"main"})
      : DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                     const llvm::Value *,
                                     const llvm::Function *, LLVMBasedICFG &>(
            ICFG),
        EntryPoints(EntryPoints) {
    DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
  }
  ~IFDSTabulationProblemPlugin() = default;

  const llvm::Value *createZeroValue() override {
    // create a special value to represent the zero value!
    return LLVMZeroValue::getInstance();
  }

  bool isZeroValue(const llvm::Value *d) const override {
    return isLLVMZeroValue(d);
  }

  void printNode(std::ostream &os, const llvm::Instruction *n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os,
                         const llvm::Value *d) const override {
    os << llvmIRToString(d);
  }

  void printMethod(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }
};

extern std::map<std::string,
                std::unique_ptr<IFDSTabulationProblemPlugin> (*)(
                    LLVMBasedICFG &I, std::vector<std::string> EntryPoints)>
    IFDSTabulationProblemPluginFactory;

} // namespace psr

#endif
