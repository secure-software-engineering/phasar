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

#include <map>
#include <set>
#include <string>

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
  // Delta matrix to implement the state machine's Delta function
  static const OpenSSLSecureHeapState Delta[5][6];

  IDESolver<IDESecureHeapPropagationAnalysisDomain>
      &SecureHeapPropagationResults;

  static OpenSSLSecureHeapToken funcNameToToken(const std::string &F);

public:
  OpenSSLSecureHeapDescription(IDESolver<IDESecureHeapPropagationAnalysisDomain>
                                   &SecureHeapPropagationResults);

  [[nodiscard]] bool isFactoryFunction(const std::string &F) const override;
  [[nodiscard]] bool isConsumingFunction(const std::string &F) const override;
  [[nodiscard]] bool isAPIFunction(const std::string &F) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(const std::string &Tok, TypeStateDescription::State S,
               const llvm::CallBase *CallSite) const override;
  [[nodiscard]] std::string getTypeNameOfInterest() const override;
  [[nodiscard]] std::set<int>
  getConsumerParamIdx(const std::string &F) const override;
  [[nodiscard]] std::set<int>
  getFactoryParamIdx(const std::string &F) const override;
  [[nodiscard]] std::string
  stateToString(TypeStateDescription::State S) const override;
  [[nodiscard]] TypeStateDescription::State bottom() const override;
  [[nodiscard]] TypeStateDescription::State top() const override;
  [[nodiscard]] TypeStateDescription::State uninit() const override;
  [[nodiscard]] TypeStateDescription::State start() const override;
  [[nodiscard]] TypeStateDescription::State error() const override;
};

} // namespace psr

#endif
