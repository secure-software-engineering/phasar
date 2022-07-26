/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREMEMORYDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREMEMORYDESCRIPTION_H

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

namespace psr {

class OpenSSLSecureMemoryDescription : public TypeStateDescription {
private:
  enum OpenSSLSecureMemoryState {
    TOP = 42,
    BOT = 0,
    ZEROED = 1,
    FREED = 2,
    ERROR = 3,
    ALLOCATED = 4
  };

  enum class OpenSSLSecureMemoryToken {
    CRYPTO_MALLOC = 0,
    CRYPTO_ZALLOC = 1,
    CRYPTO_FREE = 2,
    OPENSSL_CLEANSE = 3,
    STAR = 4
  };

  static const std::map<std::string, std::set<int>> OpenSSLSecureMemoryFuncs;
  // Delta matrix to implement the state machine's Delta function
  static const OpenSSLSecureMemoryState Delta[6][7];
  static OpenSSLSecureMemoryToken funcNameToToken(const std::string &F);

public:
  [[nodiscard]] bool isFactoryFunction(const std::string &F) const override;
  [[nodiscard]] bool isConsumingFunction(const std::string &F) const override;
  [[nodiscard]] bool isAPIFunction(const std::string &F) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;
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
