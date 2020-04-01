/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSSECUREMEMORYDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSSECUREMEMORYDESCRIPTION_H_

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
  // delta matrix to implement the state machine's delta function
  static const OpenSSLSecureMemoryState delta[6][7];
  OpenSSLSecureMemoryToken funcNameToToken(const std::string &F) const;

public:
  bool isFactoryFunction(const std::string &F) const override;
  bool isConsumingFunction(const std::string &F) const override;
  bool isAPIFunction(const std::string &F) const override;
  TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;
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
