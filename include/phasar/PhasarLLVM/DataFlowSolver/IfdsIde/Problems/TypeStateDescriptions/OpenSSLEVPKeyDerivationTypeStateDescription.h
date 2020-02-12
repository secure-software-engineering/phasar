/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKEYDERIVATIONTYPESTATEDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKEYDERIVATIONTYPESTATEDESCRIPTION_H_

#include <map>
#include <set>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>

namespace psr {

/**
 * A type state description for OpenSSL's EVP Key Derivation functions. The
 * finite state machine is encoded by a two-dimensional array with rows as
 * function tokens and columns as states.
 */
class OpenSSLEVPKeyDerivationTypeStateDescription
    : public TypeStateDescription {
private:
  /**
   * We use the following lattice
   *                BOT = all information
   *
   * UNINIT   KDF_FETCHED   CTX_ATTACHED   PARAM_INIT   DERIVED   ERROR
   *
   *                TOP = no information
   */
  enum OpenSSLEVPKeyDerivationState {
    TOP = 42,
    UNINIT = 0,
    KDF_FETCHED = 1,
    CTX_ATTACHED = 2,
    PARAM_INIT = 3,
    DERIVED = 4,
    ERROR = 5,
    BOT = 6
  };

  /**
   * The STAR token represents all functions besides EVP_KDF_fetch(),
   * EVP_KDF_CTX_new(), EVP_KDF_CTX_set_params() ,derive() and
   * EVP_KDF_CTX_free().
   */
  enum class OpenSSLEVPKeyDerivationToken {
    EVP_KDF_FETCH = 0,
    EVP_KDF_CTX_NEW = 1,
    EVP_KDF_CTX_SET_PARAMS = 2,
    DERIVE = 3,
    EVP_KDF_CTX_FREE = 4,
    STAR = 5
  };

  static const std::map<std::string, std::set<int>>
      OpenSSLEVPKeyDerivationFuncs;

  // delta matrix to implement the state machine's delta function
  static const OpenSSLEVPKeyDerivationState delta[6][7];

  OpenSSLEVPKeyDerivationToken funcNameToToken(const std::string &F) const;

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
