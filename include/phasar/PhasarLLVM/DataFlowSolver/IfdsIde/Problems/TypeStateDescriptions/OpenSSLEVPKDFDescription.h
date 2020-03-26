/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All righ reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFDESCRIPTION_H_

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

namespace psr {
class OpenSSLEVPKDFDescription : public TypeStateDescription {
public:
  /**
   * We use the following lattice
   *                BOT = all information
   *
   *         UNINIT     KDF_FETCHED     ERROR
   *
   *                TOP = no information
   */
  enum OpenSSLEVPKDFState {
    TOP = 42,
    UNINIT = 0,
    KDF_FETCHED = 1,
    ERROR = 2,
    BOT = 3
  };

  /**
   * The STAR token represents all functions besides EVP_KDF_fetch(),
   * EVP_KDF_fetch()  and EVP_KDF_CTX_free().
   */
  enum class OpenSSLEVTKDFToken {
    EVP_KDF_FETCH = 0,
    EVP_KDF_FREE = 1,
    STAR = 2
  };

private:
  static const std::map<std::string, std::set<int>> OpenSSLEVPKDFFuncs;
  // delta matrix to implement the state machine's delta function
  static const OpenSSLEVPKDFState delta[3][4];
  OpenSSLEVTKDFToken funcNameToToken(const std::string &F) const;

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