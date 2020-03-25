/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSSECUREHEAPDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSSECUREHEAPDESCRIPTION_H_

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

namespace psr {

class OpenSSLSecureHeapDescription : public TypeStateDescription {
private:
  enum OpenSSLSecureHeapState {
    TOP = 42,
    BOT = 0,
    UNINIT = 1,
    ALLOCATED = 2,
    ZEROED = 3,
    FREED = 4,
    ERROR = 5
  };

  enum class OpenSSLSecureHeapToken {
    SECURE_MALLOC = 0,
    SECURE_ZALLOC = 1,
    SECURE_FREE = 2,
    SECURE_CLEAR_FREE = 3,
    STAR = 4
  };

  static const std::map<std::string, std::set<int>> OpenSSLSecureHeapFuncs;
  // delta matrix to implement the state machine's delta function
  static const OpenSSLSecureHeapState delta[5][6];

  IDESolver<const llvm::Instruction *, SecureHeapFact, const llvm::Function *,
            const llvm::StructType *, const llvm::Value *, SecureHeapValue,
            LLVMBasedICFG> &secureHeapPropagationResults;

  OpenSSLSecureHeapToken funcNameToToken(const std::string &F) const;

public:
  OpenSSLSecureHeapDescription(
      IDESolver<const llvm::Instruction *, SecureHeapFact,
                const llvm::Function *, const llvm::StructType *,
                const llvm::Value *, SecureHeapValue, LLVMBasedICFG>
          &secureHeapPropagationResults);

  bool isFactoryFunction(const std::string &F) const override;
  bool isConsumingFunction(const std::string &F) const override;
  bool isAPIFunction(const std::string &F) const override;
  TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;
  TypeStateDescription::State
  getNextState(const std::string &Tok, TypeStateDescription::State S,
               llvm::ImmutableCallSite CS) const override;
  std::string getTypeNameOfInterest() const override;
  std::set<int> getConsumerParamIdx(const std::string &F) const override;
  std::set<int> getFactoryParamIdx(const std::string &F) const override;
  std::string stateToString(TypeStateDescription::State S) const override;
  TypeStateDescription::State bottom() const override;
  TypeStateDescription::State top() const override;
  TypeStateDescription::State uninit() const override;
  TypeStateDescription::State start() const override;
  TypeStateDescription::State error() const override;
};

} // namespace psr

#endif
