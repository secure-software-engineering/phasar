/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREHEAPDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREHEAPDESCRIPTION_H

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

#include <string>

namespace psr {
enum class OpenSSLSecureHeapState {
  TOP = 42,
  BOT = 0,
  UNINIT = 1,
  ALLOCATED = 2,
  ZEROED = 3,
  FREED = 4,
  ERROR = 5
};
llvm::StringRef to_string(OpenSSLSecureHeapState State) noexcept;

class OpenSSLSecureHeapDescription
    : public TypeStateDescription<OpenSSLSecureHeapState> {
private:
  enum class OpenSSLSecureHeapToken {
    SECURE_MALLOC = 0,
    SECURE_ZALLOC = 1,
    SECURE_FREE = 2,
    SECURE_CLEAR_FREE = 3,
    STAR = 4
  };

  // Delta matrix to implement the state machine's Delta function
  static const OpenSSLSecureHeapState Delta[5][6];

  IDESolver<IDESecureHeapPropagationAnalysisDomain>
      &SecureHeapPropagationResults;

  static OpenSSLSecureHeapToken funcNameToToken(llvm::StringRef F);

public:
  using TypeStateDescription::getNextState;
  OpenSSLSecureHeapDescription(IDESolver<IDESecureHeapPropagationAnalysisDomain>
                                   &SecureHeapPropagationResults);

  [[nodiscard]] bool isFactoryFunction(llvm::StringRef F) const override;
  [[nodiscard]] bool isConsumingFunction(llvm::StringRef F) const override;
  [[nodiscard]] bool isAPIFunction(llvm::StringRef F) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(llvm::StringRef Tok,
               TypeStateDescription::State S) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(llvm::StringRef Tok, TypeStateDescription::State S,
               const llvm::CallBase *CallSite) const override;
  [[nodiscard]] std::string getTypeNameOfInterest() const override;
  [[nodiscard]] std::set<int>
  getConsumerParamIdx(llvm::StringRef F) const override;
  [[nodiscard]] std::set<int>
  getFactoryParamIdx(llvm::StringRef F) const override;
  [[nodiscard]] TypeStateDescription::State bottom() const override;
  [[nodiscard]] TypeStateDescription::State top() const override;
  [[nodiscard]] TypeStateDescription::State uninit() const override;
  [[nodiscard]] TypeStateDescription::State start() const override;
  [[nodiscard]] TypeStateDescription::State error() const override;
};

} // namespace psr

#endif
